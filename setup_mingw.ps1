# QB64-PE LLVM-MinGW setup script.
#
# This script intentionally targets Windows PowerShell 2.0 so it can run on
# Windows 7 without requiring newer PowerShell cmdlets such as Invoke-WebRequest,
# Invoke-RestMethod, Get-CimInstance or Expand-Archive.
#
# Argument 1 may be "32" to force installation of the 32-bit LLVM-MinGW package
# on a 64-bit Windows installation.

param(
    [string]$ArchitectureOverride
)

$ErrorActionPreference = "Stop"
$setupSucceeded = $false
$setupSkipped = $false
$downloadFile = $null
$extractDirectory = $null

function Enable-Tls12IfAvailable {
    # PowerShell 2.0 commonly runs against an older .NET Framework where the
    # Tls12 enum member is not named. 3072 is the numeric value used by newer
    # frameworks. Assigning it this way lets updated Windows 7 installations
    # use TLS 1.2 without making the script depend on a newer PowerShell parser.
    try {
        $tls12 = [Enum]::ToObject([Net.SecurityProtocolType], 3072)
        [Net.ServicePointManager]::SecurityProtocol = $tls12
    }
    catch {
        # Leave the framework default unchanged. A later HTTPS operation will
        # report the real failure if this machine cannot negotiate with GitHub.
    }
}

function Get-LatestReleaseUrl {
    param([string]$LatestUrl)

    $request = $null
    $response = $null

    try {
        $request = [Net.HttpWebRequest]::Create($LatestUrl)
        $request.AllowAutoRedirect = $true
        $request.UserAgent = "QB64-PE setup_mingw.ps1"
        $request.Timeout = 30000
        $response = $request.GetResponse()
        return $response.ResponseUri.AbsoluteUri
    }
    finally {
        if ($response -ne $null) {
            $response.Close()
        }
    }
}

function Download-File {
    param(
        [string]$Url,
        [string]$Destination
    )

    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Force
    }

    $client = New-Object Net.WebClient
    try {
        $client.Headers.Add("User-Agent", "QB64-PE setup_mingw.ps1")
        $client.DownloadFile($Url, $Destination)
    }
    catch {
        if (Test-Path -LiteralPath $Destination) {
            Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
        }
        throw
    }
    finally {
        $client.Dispose()
    }

    if (-not (Test-Path -LiteralPath $Destination)) {
        throw "The download completed without creating the expected ZIP file."
    }

    $downloadInfo = Get-Item -LiteralPath $Destination
    if ($downloadInfo.Length -le 0) {
        Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
        throw "The downloaded ZIP file is empty."
    }
}

function Get-DirectoryStats {
    param([string]$Path)

    [long]$fileCount = 0
    [long]$byteCount = 0

    if (Test-Path -LiteralPath $Path) {
        $items = Get-ChildItem -LiteralPath $Path -Recurse -Force -ErrorAction SilentlyContinue
        foreach ($item in $items) {
            if (-not $item.PSIsContainer) {
                $fileCount++
                $byteCount += $item.Length
            }
        }
    }

    $result = New-Object PSObject
    $result | Add-Member -MemberType NoteProperty -Name FileCount -Value $fileCount
    $result | Add-Member -MemberType NoteProperty -Name ByteCount -Value $byteCount
    return $result
}

function Get-ShellFolderStats {
    param($Folder)

    # Shell.Application does not expose a completion event for CopyHere().
    # Build an exact manifest summary from the ZIP namespace before extraction.
    # Because the destination directory is freshly removed before each setup,
    # matching both file count and total uncompressed byte count gives us a
    # deterministic completion condition instead of guessing from quiet time.
    [long]$fileCount = 0
    [long]$byteCount = 0

    $items = $Folder.Items()
    foreach ($item in $items) {
        if ($item.IsFolder) {
            $childFolder = $item.GetFolder
            if ($childFolder -eq $null) {
                throw ("Unable to inspect ZIP directory '{0}'." -f $item.Name)
            }

            $childStats = Get-ShellFolderStats $childFolder
            $fileCount += $childStats.FileCount
            $byteCount += $childStats.ByteCount
        }
        else {
            $fileCount++
            $byteCount += [long]$item.Size
        }
    }

    $result = New-Object PSObject
    $result | Add-Member -MemberType NoteProperty -Name FileCount -Value $fileCount
    $result | Add-Member -MemberType NoteProperty -Name ByteCount -Value $byteCount
    return $result
}

function Expand-Zip {
    param(
        [string]$ZipPath,
        [string]$Destination,
        [string]$ExpectedDirectory
    )

    # Prefer the synchronous ZipFile API when the installed .NET Framework
    # provides it. This remains valid PowerShell 2.0 syntax; older Windows 7
    # systems simply fall through to the built-in Windows ZIP shell namespace.
    try {
        Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction Stop | Out-Null
        [IO.Compression.ZipFile]::ExtractToDirectory($ZipPath, $Destination)
        return
    }
    catch {
        if (Test-Path -LiteralPath $ExpectedDirectory) {
            Remove-Item -LiteralPath $ExpectedDirectory -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    Expand-ZipWithShell $ZipPath $Destination $ExpectedDirectory
}

function Expand-ZipWithShell {
    param(
        [string]$ZipPath,
        [string]$Destination,
        [string]$ExpectedDirectory
    )

    # Windows 7 has the ZIP shell namespace even though it has no tar.exe and
    # PowerShell 2.0 has no Expand-Archive. Shell.Application CopyHere() is
    # asynchronous and has no completion event, so do not use a fixed delay or
    # a "directory stopped growing" heuristic. First obtain the exact number
    # of files and total uncompressed bytes represented by the ZIP namespace,
    # then wait until the fresh extraction tree matches both values.
    $shell = New-Object -ComObject Shell.Application
    $zipNamespace = $shell.NameSpace($ZipPath)
    $destinationNamespace = $shell.NameSpace($Destination)

    if (($zipNamespace -eq $null) -or ($destinationNamespace -eq $null)) {
        throw "Windows ZIP shell support is unavailable."
    }

    $expectedStats = Get-ShellFolderStats $zipNamespace
    if ($expectedStats.FileCount -le 0) {
        throw "The downloaded ZIP file contains no files."
    }

    # 4 = suppress progress UI, 16 = suppress confirmation prompts.
    $destinationNamespace.CopyHere($zipNamespace.Items(), 20)

    [long]$lastFileCount = -1
    [long]$lastByteCount = -1
    $startTime = [DateTime]::UtcNow
    $lastProgressTime = $startTime

    # A slow disk is allowed to take as long as needed while observable
    # progress continues. Only abort after ten minutes with no change at all,
    # with a one-hour absolute guard against a permanently stuck shell copy.
    while ($true) {
        Start-Sleep -Seconds 4

        $currentStats = Get-DirectoryStats $ExpectedDirectory

        if (($currentStats.FileCount -eq $expectedStats.FileCount) -and
            ($currentStats.ByteCount -eq $expectedStats.ByteCount)) {
            return
        }

        if (($currentStats.FileCount -ne $lastFileCount) -or
            ($currentStats.ByteCount -ne $lastByteCount)) {
            $lastFileCount = $currentStats.FileCount
            $lastByteCount = $currentStats.ByteCount
            $lastProgressTime = [DateTime]::UtcNow
        }

        $now = [DateTime]::UtcNow
        if (($now - $lastProgressTime).TotalMinutes -ge 10) {
            throw ("LLVM-MinGW extraction stopped making progress ({0}/{1} files, {2}/{3} bytes)." -f `
                $currentStats.FileCount, $expectedStats.FileCount, $currentStats.ByteCount, $expectedStats.ByteCount)
        }

        if (($now - $startTime).TotalHours -ge 1) {
            throw ("Timed out while extracting LLVM-MinGW ({0}/{1} files, {2}/{3} bytes)." -f `
                $currentStats.FileCount, $expectedStats.FileCount, $currentStats.ByteCount, $expectedStats.ByteCount)
        }
    }
}

try {
    # $PSScriptRoot was added after PowerShell 2.0. Resolve the script directory
    # through $MyInvocation instead so this script works on an unmodified Win7.
    $scriptPath = $MyInvocation.MyCommand.Path
    if ([String]::IsNullOrEmpty($scriptPath)) {
        throw "Unable to determine the setup script path."
    }

    $rootDirectory = Split-Path -Parent $scriptPath
    Set-Location -LiteralPath $rootDirectory

    $compilerDirectory = Join-Path $rootDirectory "internal\c\c_compiler"
    $compilerExe = Join-Path $compilerDirectory "bin\c++.exe"

    if (Test-Path -LiteralPath $compilerExe) {
        Write-Host ""
        Write-Host "Info: LLVM-MinGW detected. Skipping setup."
        $setupSucceeded = $true
        $setupSkipped = $true
    }
    else {
        if (-not (Test-Path -LiteralPath $compilerDirectory)) {
            New-Item -ItemType Directory -Path $compilerDirectory -Force | Out-Null
        }

        if (-not (Test-Path -LiteralPath $compilerDirectory)) {
            throw "Not able to create 'internal\c\c_compiler\'."
        }

        # PROCESSOR_ARCHITEW6432 identifies the native architecture when a
        # 32-bit PowerShell process is running under WOW64 on 64-bit Windows.
        $processArchitecture = [Environment]::GetEnvironmentVariable("PROCESSOR_ARCHITECTURE")
        $nativeArchitecture = [Environment]::GetEnvironmentVariable("PROCESSOR_ARCHITEW6432")

        $cpuArchitecture = $null
        $osBits = 32

        if (($processArchitecture -eq "x86") -or
            ($processArchitecture -eq "AMD64") -or
            ($processArchitecture -eq "IA64")) {
            $cpuArchitecture = "X86"
        }
        elseif (($processArchitecture -eq "ARM") -or
                ($processArchitecture -eq "ARM64")) {
            $cpuArchitecture = "ARM"
        }

        if (($nativeArchitecture -eq "AMD64") -or ($nativeArchitecture -eq "ARM64")) {
            if ($nativeArchitecture -eq "AMD64") {
                $cpuArchitecture = "X86"
            }
            else {
                $cpuArchitecture = "ARM"
            }
            $osBits = 64
        }
        elseif (($processArchitecture -eq "AMD64") -or
                ($processArchitecture -eq "IA64") -or
                ($processArchitecture -eq "ARM64")) {
            $osBits = 64
        }

        if ([String]::IsNullOrEmpty($cpuArchitecture)) {
            throw "Unknown processor type '$processArchitecture'."
        }

        if ($ArchitectureOverride -eq "32") {
            $osBits = 32
        }

        Write-Host ("Platform selected: {0}-{1}" -f $cpuArchitecture, $osBits)

        # The UCRT package is intentionally retained to match QB64-PE's current
        # LLVM-MinGW selection. Windows 7 needs the Universal CRT update because
        # ucrtbase.dll is not part of the original Windows 7 installation.
        $windowsVersion = [Environment]::OSVersion.Version
        if (($windowsVersion.Major -eq 6) -and ($windowsVersion.Minor -eq 1)) {
            $ucrtPath = Join-Path $env:SystemRoot "System32\ucrtbase.dll"
            if (-not (Test-Path -LiteralPath $ucrtPath)) {
                Write-Host ""
                Write-Host "Warning: Universal CRT was not detected on this Windows 7 system."
                Write-Host "         LLVM-MinGW may not run until the Windows Universal CRT update is installed."
                Write-Host ""
            }
        }

        Enable-Tls12IfAvailable

        $latestReleaseUrl = "https://github.com/mstorsjo/llvm-mingw/releases/latest"
        try {
            $releaseUrl = Get-LatestReleaseUrl $latestReleaseUrl
        }
        catch {
            throw "Unable to detect latest LLVM-MinGW release. Windows 7 requires working TLS 1.2 support to access GitHub. $($_.Exception.Message)"
        }

        if ([String]::IsNullOrEmpty($releaseUrl)) {
            throw "Unable to detect latest LLVM-MinGW release."
        }

        $lastSlash = $releaseUrl.LastIndexOf('/')
        if (($lastSlash -lt 0) -or ($lastSlash -ge ($releaseUrl.Length - 1))) {
            throw "Unable to determine the LLVM-MinGW release tag."
        }

        $releaseTag = $releaseUrl.Substring($lastSlash + 1)
        if ([String]::IsNullOrEmpty($releaseTag)) {
            throw "Unable to determine the LLVM-MinGW release tag."
        }

        Write-Host ("LLVM-MinGW release detected: {0}" -f $releaseTag)

        if ($cpuArchitecture -eq "ARM") {
            if ($osBits -eq 64) {
                $llvmTarget = "aarch64"
            }
            else {
                $llvmTarget = "armv7"
            }
        }
        else {
            if ($osBits -eq 64) {
                $llvmTarget = "x86_64"
            }
            else {
                $llvmTarget = "i686"
            }
        }

        Write-Host ("LLVM-MinGW target selected: {0}" -f $llvmTarget)

        $llvmDirectoryName = "llvm-mingw-{0}-ucrt-{1}" -f $releaseTag, $llvmTarget
        $downloadUrl = "https://github.com/mstorsjo/llvm-mingw/releases/download/{0}/{1}.zip" -f $releaseTag, $llvmDirectoryName
        $downloadFile = Join-Path $rootDirectory "temp.zip"
        $extractDirectory = Join-Path $rootDirectory $llvmDirectoryName

        Write-Host ("Download URL: {0}" -f $downloadUrl)

        if (Test-Path -LiteralPath $extractDirectory) {
            Remove-Item -LiteralPath $extractDirectory -Recurse -Force
        }

        Write-Host ("Downloading {0}..." -f $downloadUrl)
        try {
            Download-File $downloadUrl $downloadFile
        }
        catch {
            throw "Unable to download LLVM-MinGW. On Windows 7, make sure TLS 1.2 is enabled and GitHub is reachable. $($_.Exception.Message)"
        }

        Write-Host "Extracting C++ Compiler..."
        Expand-Zip $downloadFile $rootDirectory $extractDirectory

        $extractedCompiler = Join-Path $extractDirectory "bin\c++.exe"
        if (-not (Test-Path -LiteralPath $extractedCompiler)) {
            throw "LLVM-MinGW extraction completed without the expected c++.exe."
        }

        Write-Host "Moving C++ compiler..."
        $extractedItems = Get-ChildItem -LiteralPath $extractDirectory -Force
        foreach ($item in $extractedItems) {
            Move-Item -LiteralPath $item.FullName -Destination $compilerDirectory -Force
        }

        if (-not (Test-Path -LiteralPath $compilerExe)) {
            throw "LLVM-MinGW extraction completed, but c++.exe was not installed."
        }

        $setupSucceeded = $true
    }
}
catch {
    Write-Host ""
    Write-Host ("Error: {0}" -f $_.Exception.Message)
}
finally {
    # Always remove setup-owned temporary data. Never remove c_compiler itself,
    # because an existing partial/custom tree may predate this invocation.
    $cleanupNeeded = $false
    if (($extractDirectory -ne $null) -and (Test-Path -LiteralPath $extractDirectory)) {
        $cleanupNeeded = $true
    }
    if (($downloadFile -ne $null) -and (Test-Path -LiteralPath $downloadFile)) {
        $cleanupNeeded = $true
    }

    if ($cleanupNeeded) {
        Write-Host "Cleaning up..."
    }

    if (($extractDirectory -ne $null) -and (Test-Path -LiteralPath $extractDirectory)) {
        Remove-Item -LiteralPath $extractDirectory -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (($downloadFile -ne $null) -and (Test-Path -LiteralPath $downloadFile)) {
        Remove-Item -LiteralPath $downloadFile -Force -ErrorAction SilentlyContinue
    }
}

if ($setupSucceeded) {
    if (-not $setupSkipped) {
        Write-Host ""
        Write-Host "LLVM-MinGW setup completed successfully."
    }
    exit 0
}

exit 1
