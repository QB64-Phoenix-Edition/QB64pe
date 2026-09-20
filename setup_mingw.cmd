@rem QB64-PE LLVM-MinGW setup script
@rem
@rem This NT command script downloads and extracts the latest copy of LLVM-MinGW binaries from:
@rem https://github.com/mstorsjo/llvm-mingw/releases/latest
@rem
@rem Specifying 32 for argument 1 on a 64-bit system will force a 32-bit LLVM-MinGW setup.
@rem
@rem Windows 7 compatibility:
@rem - Does not require PowerShell.
@rem - Detects the native architecture from Windows environment variables.
@rem - First uses a working curl.exe from PATH when available.
@rem - Then uses curl.exe/unzip.exe from a full Git for Windows installation.
@rem - Finally, if only GitHub Desktop is available, uses GitHubDesktop.exe as
@rem   an Electron/Node HTTPS runtime and Windows Shell.Application for ZIP extraction.
@rem - Does not store curl.exe, CA bundles, or other helper binaries in this repository.
@rem
@echo off

rem Enable cmd extensions and exit if not present
setlocal EnableExtensions
if errorlevel 1 (
    echo.
    echo Error: Command Prompt extensions not available!
    goto failed
)

set "SETUP_RESULT=1"
set "LLVM_RELEASE_FILE=.qb64pe_llvm_release.tmp"
set "MINGW_TEMP_FILE=temp.zip"
set "ZIP_HELPER=.qb64pe_unzip.vbs"

rem Change to the correct drive & path
cd /d "%~dp0"
set "MINGW_TEMP_PATH=%CD%\%MINGW_TEMP_FILE%"

rem Check if the C++ compiler is there and skip downloading if it exists
if exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Info: LLVM-MinGW detected. Skipping setup.
    set "SETUP_RESULT=0"
    goto end
)

rem Create the c_compiler directory that should contain the LLVM-MinGW binaries
if not exist "internal\c\c_compiler\" mkdir "internal\c\c_compiler"

rem Check if we were able to create the directory
if not exist "internal\c\c_compiler\" (
    echo.
    echo Error: Not able to create 'internal\c\c_compiler\'!
    goto failed
)

rem Detect the native Windows architecture without PowerShell/WMI.
rem PROCESSOR_ARCHITEW6432 contains the native architecture when a 32-bit process
rem is running under WOW64 on 64-bit Windows.
set "NATIVE_ARCH=%PROCESSOR_ARCHITECTURE%"
if defined PROCESSOR_ARCHITEW6432 set "NATIVE_ARCH=%PROCESSOR_ARCHITEW6432%"

set "CPU_ARCH="
set "OS_BITS=32"

if /I "%NATIVE_ARCH%"=="AMD64" set "CPU_ARCH=X86"
if /I "%NATIVE_ARCH%"=="AMD64" set "OS_BITS=64"
if /I "%NATIVE_ARCH%"=="x86" set "CPU_ARCH=X86"
if /I "%NATIVE_ARCH%"=="ARM64" set "CPU_ARCH=ARM"
if /I "%NATIVE_ARCH%"=="ARM64" set "OS_BITS=64"
if /I "%NATIVE_ARCH%"=="ARM" set "CPU_ARCH=ARM"

if not defined CPU_ARCH (
    echo.
    echo Error: Unknown processor type '%NATIVE_ARCH%'!
    goto failed
)

rem Allow forcing 32-bit
if "%~1"=="32" set "OS_BITS=32"

echo Platform selected: %CPU_ARCH%-%OS_BITS%

if /I "%CPU_ARCH%"=="ARM" goto select_arm_target
if "%OS_BITS%"=="64" set "LLVM_TARGET=x86_64"
if "%OS_BITS%"=="32" set "LLVM_TARGET=i686"
goto target_selected

:select_arm_target
if "%OS_BITS%"=="64" set "LLVM_TARGET=aarch64"
if "%OS_BITS%"=="32" set "LLVM_TARGET=armv7"

:target_selected
if not defined LLVM_TARGET (
    echo.
    echo Error: Unable to select LLVM-MinGW target!
    goto failed
)

echo LLVM-MinGW target selected: %LLVM_TARGET%

rem Find the available download and extraction paths.
call :find_tools

rem Detect the latest LLVM-MinGW release.
set "LLVM_RELEASE_TAG="
set "LLVM_RELEASE_URL="
set "CURL_EXE="
set "DOWNLOAD_MODE="

rem Route 1: a normal curl.exe already available in PATH.
if defined PATH_CURL_EXE (
    echo Trying cURL from PATH for GitHub access...
    call :detect_release_curl "%PATH_CURL_EXE%"
)
if defined LLVM_RELEASE_TAG goto release_detected

rem Route 2: a full Git for Windows installation discovered from git.exe/registry.
if defined GIT_CURL_EXE (
    if /I not "%GIT_CURL_EXE%"=="%PATH_CURL_EXE%" (
        echo Trying cURL bundled with Git for Windows...
        call :detect_release_curl "%GIT_CURL_EXE%"
    )
)
if defined LLVM_RELEASE_TAG goto release_detected

rem Route 3: GitHub Desktop only. Its Electron executable can run as Node and
rem perform HTTPS without using PowerShell/.NET download code.
if defined GITHUB_DESKTOP_EXE (
    echo Trying GitHub Desktop Electron/Node for GitHub access...
    call :detect_release_desktop
)
if defined LLVM_RELEASE_TAG goto release_detected

rem No usable transport was found.
echo.
echo Error: Unable to detect the latest LLVM-MinGW release.
echo.
echo Tried, in order:
echo   1. curl.exe from PATH
echo   2. curl.exe from a full Git for Windows installation
echo   3. GitHubDesktop.exe as an Electron/Node HTTPS runtime
echo.
echo On Windows 7, use Git for Windows v2.46.2 ^(the last version supporting
echo Windows 7^), GitHub Desktop 3.2.6, or another TLS-capable curl.exe.
goto failed

:release_detected
echo LLVM-MinGW release detected: %LLVM_RELEASE_TAG%
if /I "%DOWNLOAD_MODE%"=="CURL" echo Download helper: "%CURL_EXE%"
if /I "%DOWNLOAD_MODE%"=="DESKTOP" echo Download helper: "%GITHUB_DESKTOP_EXE%" ^(Electron/Node^)

rem Build directory and download URL
set "LLVM_DIR_NAME=llvm-mingw-%LLVM_RELEASE_TAG%-ucrt-%LLVM_TARGET%"
set "LLVM_DOWNLOAD_URL=https://github.com/mstorsjo/llvm-mingw/releases/download/%LLVM_RELEASE_TAG%/%LLVM_DIR_NAME%.zip"

echo Download URL: %LLVM_DOWNLOAD_URL%

if defined MINGW_TEMP_PATH if exist "%MINGW_TEMP_PATH%" del /q "%MINGW_TEMP_PATH%" >nul 2>nul

rem Download LLVM-MinGW using whichever transport detected the release.
echo Downloading LLVM-MinGW...
if /I "%DOWNLOAD_MODE%"=="CURL" goto download_with_curl
if /I "%DOWNLOAD_MODE%"=="DESKTOP" goto download_with_desktop

echo.
echo Error: No download mode was selected!
goto failed

:download_with_curl
"%CURL_EXE%" -fL --retry 2 "%LLVM_DOWNLOAD_URL%" -o "%MINGW_TEMP_PATH%"
if errorlevel 1 (
    echo.
    echo Error: LLVM-MinGW download failed!
    goto failed
)
goto download_complete

:download_with_desktop
call :download_desktop "%LLVM_DOWNLOAD_URL%" "%MINGW_TEMP_PATH%"
if errorlevel 1 (
    echo.
    echo Error: LLVM-MinGW download through GitHub Desktop failed!
    goto failed
)

goto download_complete

:download_complete
if not exist "%MINGW_TEMP_PATH%" (
    echo.
    echo Error: Download completed without creating "%MINGW_TEMP_PATH%"!
    goto failed
)

rem Prefer normal extraction tools when available. The Shell.Application path
rem is primarily for a Windows 7 machine that has GitHub Desktop but not full Git.
if defined UNZIP_EXE goto extract_with_unzip
if defined TAR_EXE goto extract_with_tar
if defined CSCRIPT_EXE goto extract_with_shell
goto no_extractor

:extract_with_unzip
echo Extracting C++ compiler using "%UNZIP_EXE%"...
"%UNZIP_EXE%" -q "%MINGW_TEMP_PATH%"
if not errorlevel 1 if exist "%LLVM_DIR_NAME%\" goto extracted

echo Warning: unzip.exe failed. Trying another extraction method...
if exist "%LLVM_DIR_NAME%\" rd /s /q "%LLVM_DIR_NAME%" >nul 2>nul
if defined TAR_EXE goto extract_with_tar
if defined CSCRIPT_EXE goto extract_with_shell
goto extract_failed

:extract_with_tar
echo Extracting C++ compiler using "%TAR_EXE%"...
"%TAR_EXE%" -xf "%MINGW_TEMP_PATH%"
if not errorlevel 1 if exist "%LLVM_DIR_NAME%\" goto extracted

echo Warning: tar.exe failed. Trying Windows Shell.Application...
if exist "%LLVM_DIR_NAME%\" rd /s /q "%LLVM_DIR_NAME%" >nul 2>nul
if defined CSCRIPT_EXE goto extract_with_shell
goto extract_failed

:extract_with_shell
echo Extracting C++ compiler using Windows Shell.Application...
call :write_zip_helper
if errorlevel 1 goto extract_failed

"%CSCRIPT_EXE%" //nologo "%ZIP_HELPER%" "%MINGW_TEMP_PATH%" "%CD%" "%LLVM_DIR_NAME%"
set "SHELL_EXTRACT_RESULT=%ERRORLEVEL%"
if exist "%ZIP_HELPER%" del /q "%ZIP_HELPER%" >nul 2>nul

if "%SHELL_EXTRACT_RESULT%"=="0" if exist "%LLVM_DIR_NAME%\bin\c++.exe" goto extracted
if exist "%LLVM_DIR_NAME%\" rd /s /q "%LLVM_DIR_NAME%" >nul 2>nul
goto extract_failed

:no_extractor
echo.
echo Error: No ZIP extraction method was found.
echo Expected unzip.exe, tar.exe, or the Windows cscript/Shell.Application path.
goto failed

:extract_failed
echo.
echo Error: Unable to extract the LLVM-MinGW archive!
goto failed

:extracted
rem Move the binaries to internal\c\c_compiler\
echo Moving C++ compiler...
for /f "delims=" %%a in ('dir "%LLVM_DIR_NAME%" /b') do move /y "%LLVM_DIR_NAME%\%%a" "internal\c\c_compiler\" >nul

if not exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Error: LLVM-MinGW was extracted, but c++.exe was not installed correctly!
    goto failed
)

rem Cleanup downloaded temporary files
echo Cleaning up...
if exist "%LLVM_DIR_NAME%\" rd /s /q "%LLVM_DIR_NAME%" >nul 2>nul
if defined MINGW_TEMP_PATH if exist "%MINGW_TEMP_PATH%" del /q "%MINGW_TEMP_PATH%" >nul 2>nul
if exist "%LLVM_RELEASE_FILE%" del /q "%LLVM_RELEASE_FILE%" >nul 2>nul
if exist "%ZIP_HELPER%" del /q "%ZIP_HELPER%" >nul 2>nul

set "SETUP_RESULT=0"
goto end

:failed
set "SETUP_RESULT=1"
if defined LLVM_RELEASE_FILE if exist "%LLVM_RELEASE_FILE%" del /q "%LLVM_RELEASE_FILE%" >nul 2>nul
if defined MINGW_TEMP_FILE if defined MINGW_TEMP_PATH if exist "%MINGW_TEMP_PATH%" del /q "%MINGW_TEMP_PATH%" >nul 2>nul
if defined ZIP_HELPER if exist "%ZIP_HELPER%" del /q "%ZIP_HELPER%" >nul 2>nul

:end
endlocal & exit /b %SETUP_RESULT%

rem ---------------------------------------------------------------------------
rem Helper: detect the latest GitHub release through the supplied curl.exe.
rem On success CURL_EXE, DOWNLOAD_MODE and LLVM_RELEASE_TAG are set.
rem ---------------------------------------------------------------------------
:detect_release_curl
set "LLVM_RELEASE_URL="
set "LLVM_RELEASE_TAG="
if exist "%LLVM_RELEASE_FILE%" del /q "%LLVM_RELEASE_FILE%" >nul 2>nul

"%~1" -fsSL -o NUL -w "%%{url_effective}" "https://github.com/mstorsjo/llvm-mingw/releases/latest" > "%LLVM_RELEASE_FILE%" 2>nul
if errorlevel 1 goto detect_release_curl_failed

set /p "LLVM_RELEASE_URL="<"%LLVM_RELEASE_FILE%"
del /q "%LLVM_RELEASE_FILE%" >nul 2>nul

if not defined LLVM_RELEASE_URL goto detect_release_curl_failed

rem GitHub redirects /releases/latest to /releases/tag/<tag>.
set "LLVM_RELEASE_TAG=%LLVM_RELEASE_URL:https://github.com/mstorsjo/llvm-mingw/releases/tag/=%"
if "%LLVM_RELEASE_TAG%"=="%LLVM_RELEASE_URL%" goto detect_release_curl_failed
if "%LLVM_RELEASE_TAG:~-1%"=="/" set "LLVM_RELEASE_TAG=%LLVM_RELEASE_TAG:~0,-1%"
if not defined LLVM_RELEASE_TAG goto detect_release_curl_failed

set "CURL_EXE=%~1"
set "DOWNLOAD_MODE=CURL"
exit /b 0

:detect_release_curl_failed
set "LLVM_RELEASE_URL="
set "LLVM_RELEASE_TAG="
exit /b 1

rem ---------------------------------------------------------------------------
rem Helper: use GitHub Desktop's Electron executable as a Node.js runtime.
rem This follows GitHub's /releases/latest redirect and prints only the tag.
rem ---------------------------------------------------------------------------
:detect_release_desktop
set "LLVM_RELEASE_TAG="
set "ELECTRON_RUN_AS_NODE=1"

if exist "%LLVM_RELEASE_FILE%" del /q "%LLVM_RELEASE_FILE%" >nul 2>nul

"%GITHUB_DESKTOP_EXE%" -e "var h=require('https'),U=require('url').URL;function g(u,n){var q=h.get(u,{headers:{'User-Agent':'QB64PE-setup'}},function(r){var c=r.statusCode||0;if((c==301||c==302||c==303||c==307||c==308)&&r.headers.location&&n<8){var v=new U(r.headers.location,u).toString();r.resume();g(v,n+1);return;}var p=new U(u).pathname.split('/');var t=p[p.length-1]||p[p.length-2];r.resume();if(c>=200&&c<400&&t){console.log(t);return;}process.exitCode=2;});q.on('error',function(){process.exitCode=3;});}g(process.argv[1],0);" "https://github.com/mstorsjo/llvm-mingw/releases/latest" > "%LLVM_RELEASE_FILE%" 2>nul
if errorlevel 1 goto detect_release_desktop_failed

set /p "LLVM_RELEASE_TAG="<"%LLVM_RELEASE_FILE%"
del /q "%LLVM_RELEASE_FILE%" >nul 2>nul
if not defined LLVM_RELEASE_TAG goto detect_release_desktop_failed

set "DOWNLOAD_MODE=DESKTOP"
exit /b 0

:detect_release_desktop_failed
set "LLVM_RELEASE_TAG="
if exist "%LLVM_RELEASE_FILE%" del /q "%LLVM_RELEASE_FILE%" >nul 2>nul
exit /b 1

rem ---------------------------------------------------------------------------
rem Helper: download one URL through GitHub Desktop's embedded Electron/Node.
rem Redirects are followed without relying on the Windows PowerShell/.NET stack.
rem ---------------------------------------------------------------------------
:download_desktop
set "ELECTRON_RUN_AS_NODE=1"

"%GITHUB_DESKTOP_EXE%" -e "var h=require('https'),f=require('fs'),U=require('url').URL,p=process.argv[2];console.log('Electron/Node cwd: '+process.cwd());console.log('Download file: '+p);function g(u,n){var q=h.get(u,{headers:{'User-Agent':'QB64PE-setup'}},function(r){var c=r.statusCode||0;if((c==301||c==302||c==303||c==307||c==308)&&r.headers.location&&n<8){var v=new U(r.headers.location,u).toString();r.resume();g(v,n+1);return;}if(c!=200){r.resume();process.exitCode=4;return;}var o=f.createWriteStream(p);o.on('error',function(e){console.error('File write error: '+e.message);process.exitCode=5;});r.on('error',function(e){console.error('HTTPS stream error: '+e.message);process.exitCode=6;});o.on('close',function(){try{var s=f.statSync(p);console.log('Downloaded bytes: '+s.size);if(!s.size)process.exitCode=8;}catch(e){console.error('Downloaded file check failed: '+e.message);process.exitCode=9;}});r.pipe(o);});q.on('error',function(e){console.error('HTTPS request error: '+e.message);process.exitCode=7;});}g(process.argv[1],0);" "%~1" "%~2"
exit /b %ERRORLEVEL%

rem ---------------------------------------------------------------------------
rem Helper: find download/extraction tools.
rem Priority is deliberately:
rem   1. curl.exe already in PATH
rem   2. full Git for Windows discovered through git.exe/registry/install paths
rem   3. GitHub Desktop as the final HTTPS fallback
rem ---------------------------------------------------------------------------
:find_tools
set "PATH_CURL_EXE="
set "PATH_UNZIP_EXE="
set "PATH_TAR_EXE="
set "GIT_CURL_EXE="
set "GIT_UNZIP_EXE="
set "GIT_TAR_EXE="
set "GITHUB_DESKTOP_EXE="
set "UNZIP_EXE="
set "TAR_EXE="
set "CSCRIPT_EXE="

rem First use ordinary tools visible to CMD through PATH.
for /f "delims=" %%a in ('where curl.exe 2^>nul') do if not defined PATH_CURL_EXE set "PATH_CURL_EXE=%%a"
for /f "delims=" %%a in ('where unzip.exe 2^>nul') do if not defined PATH_UNZIP_EXE set "PATH_UNZIP_EXE=%%a"
for /f "delims=" %%a in ('where tar.exe 2^>nul') do if not defined PATH_TAR_EXE set "PATH_TAR_EXE=%%a"
for /f "delims=" %%a in ('where cscript.exe 2^>nul') do if not defined CSCRIPT_EXE set "CSCRIPT_EXE=%%a"

rem Then inspect git.exe from PATH. A full Git for Windows install exposes its
rem own mingwXX\bin\curl.exe and usr\bin\unzip.exe. GitHub Desktop's MinGit
rem does not satisfy these probes, so it is not mistaken for full Git.
for /f "delims=" %%g in ('where git.exe 2^>nul') do call :probe_git_exe "%%g"

rem Git for Windows installer registry entries are fallbacks for installations
rem where git.exe itself was not added to PATH.
for /f "tokens=2,*" %%a in ('reg query "HKCU\Software\GitForWindows" /v InstallPath 2^>nul') do if /I "%%a"=="REG_SZ" call :probe_git_root "%%b"
for /f "tokens=2,*" %%a in ('reg query "HKLM\Software\GitForWindows" /v InstallPath 2^>nul') do if /I "%%a"=="REG_SZ" call :probe_git_root "%%b"
for /f "tokens=2,*" %%a in ('reg query "HKLM\Software\GitForWindows" /v InstallPath /reg:64 2^>nul') do if /I "%%a"=="REG_SZ" call :probe_git_root "%%b"
for /f "tokens=2,*" %%a in ('reg query "HKLM\Software\GitForWindows" /v InstallPath /reg:32 2^>nul') do if /I "%%a"=="REG_SZ" call :probe_git_root "%%b"

rem Final full-Git fallback for common installer locations.
if not "%ProgramW6432%"=="" call :probe_git_root "%ProgramW6432%\Git"
if not "%ProgramFiles%"=="" call :probe_git_root "%ProgramFiles%\Git"
if not "%ProgramFiles(x86)%"=="" call :probe_git_root "%ProgramFiles(x86)%\Git"
if not "%LocalAppData%"=="" call :probe_git_root "%LocalAppData%\Programs\Git"

rem Only after the normal Git route has been checked, look for GitHub Desktop.
rem Start with WHERE as requested.
for /f "delims=" %%d in ('where GitHubDesktop.exe 2^>nul') do call :probe_desktop_exe "%%d"

rem For a normal Squirrel installation prefer the real versioned Electron
rem executable in app-* over the root launcher. This also avoids relying on
rem whatever working directory behavior the root launcher happens to use.
if not defined GITHUB_DESKTOP_EXE if defined LocalAppData (
    for /f "delims=" %%d in ('dir /b /ad /o-d "%LocalAppData%\GitHubDesktop\app-*" 2^>nul') do (
        if not defined GITHUB_DESKTOP_EXE call :probe_desktop_exe "%LocalAppData%\GitHubDesktop\%%d\GitHubDesktop.exe"
    )
)

rem Root launcher is the final GitHub Desktop location fallback.
if not defined GITHUB_DESKTOP_EXE if defined LocalAppData call :probe_desktop_exe "%LocalAppData%\GitHubDesktop\GitHubDesktop.exe"

if defined PATH_UNZIP_EXE set "UNZIP_EXE=%PATH_UNZIP_EXE%"
if not defined UNZIP_EXE if defined GIT_UNZIP_EXE set "UNZIP_EXE=%GIT_UNZIP_EXE%"

if defined PATH_TAR_EXE set "TAR_EXE=%PATH_TAR_EXE%"
if not defined TAR_EXE if defined GIT_TAR_EXE set "TAR_EXE=%GIT_TAR_EXE%"

exit /b 0

rem Probe Git root based on a git.exe path. Standard Git for Windows places
rem git.exe either in <Git>\cmd, <Git>\bin, or <Git>\mingwXX\bin.
:probe_git_exe
for %%d in ("%~dp1..") do call :probe_git_root "%%~fd"
for %%d in ("%~dp1..\..") do call :probe_git_root "%%~fd"
exit /b 0

rem Probe a possible full Git for Windows installation root.
:probe_git_root
if "%~1"=="" exit /b 0
if not exist "%~1\" exit /b 0

if not defined GIT_CURL_EXE if exist "%~1\mingw64\bin\curl.exe" set "GIT_CURL_EXE=%~1\mingw64\bin\curl.exe"
if not defined GIT_CURL_EXE if exist "%~1\mingw32\bin\curl.exe" set "GIT_CURL_EXE=%~1\mingw32\bin\curl.exe"
if not defined GIT_UNZIP_EXE if exist "%~1\usr\bin\unzip.exe" set "GIT_UNZIP_EXE=%~1\usr\bin\unzip.exe"
if not defined GIT_TAR_EXE if exist "%~1\usr\bin\tar.exe" set "GIT_TAR_EXE=%~1\usr\bin\tar.exe"
exit /b 0

rem Verify that a GitHubDesktop.exe candidate really accepts Electron's Node mode.
:probe_desktop_exe
if defined GITHUB_DESKTOP_EXE exit /b 0
if "%~1"=="" exit /b 0
if not exist "%~1" exit /b 0

set "ELECTRON_RUN_AS_NODE=1"
"%~1" -e "process.exit(process.versions.electron?0:1);" >nul 2>nul
if errorlevel 1 exit /b 0

set "GITHUB_DESKTOP_EXE=%~1"
exit /b 0

rem Create a tiny VBScript that asks Windows Shell.Application to extract ZIP.
rem CopyHere is asynchronous, so the helper waits until the extracted tree size
rem has remained stable for several seconds and c++.exe exists.
:write_zip_helper
if exist "%ZIP_HELPER%" del /q "%ZIP_HELPER%" >nul 2>nul

>"%ZIP_HELPER%" echo Option Explicit
>>"%ZIP_HELPER%" echo Dim sh, fso, src, dst, target, i, lastSize, curSize, stableCount
>>"%ZIP_HELPER%" echo Set sh = CreateObject("Shell.Application")
>>"%ZIP_HELPER%" echo Set fso = CreateObject("Scripting.FileSystemObject")
>>"%ZIP_HELPER%" echo Set src = sh.NameSpace(WScript.Arguments(0))
>>"%ZIP_HELPER%" echo Set dst = sh.NameSpace(WScript.Arguments(1))
>>"%ZIP_HELPER%" echo If src Is Nothing Then WScript.Quit 2
>>"%ZIP_HELPER%" echo If dst Is Nothing Then WScript.Quit 3
>>"%ZIP_HELPER%" echo dst.CopyHere src.Items, 20
>>"%ZIP_HELPER%" echo target = fso.BuildPath(WScript.Arguments(1), WScript.Arguments(2))
>>"%ZIP_HELPER%" echo lastSize = -1
>>"%ZIP_HELPER%" echo stableCount = 0
>>"%ZIP_HELPER%" echo For i = 1 To 2400
>>"%ZIP_HELPER%" echo     WScript.Sleep 250
>>"%ZIP_HELPER%" echo     curSize = -1
>>"%ZIP_HELPER%" echo     On Error Resume Next
>>"%ZIP_HELPER%" echo     If fso.FolderExists(target) Then curSize = fso.GetFolder(target).Size
>>"%ZIP_HELPER%" echo     On Error GoTo 0
>>"%ZIP_HELPER%" echo     If curSize = lastSize Then
>>"%ZIP_HELPER%" echo         If fso.FileExists(fso.BuildPath(target, "bin\c++.exe")) Then stableCount = stableCount + 1
>>"%ZIP_HELPER%" echo     Else
>>"%ZIP_HELPER%" echo         stableCount = 0
>>"%ZIP_HELPER%" echo     End If
>>"%ZIP_HELPER%" echo     If stableCount = 20 Then WScript.Quit 0
>>"%ZIP_HELPER%" echo     lastSize = curSize
>>"%ZIP_HELPER%" echo Next
>>"%ZIP_HELPER%" echo WScript.Quit 4

if not exist "%ZIP_HELPER%" exit /b 1
exit /b 0
