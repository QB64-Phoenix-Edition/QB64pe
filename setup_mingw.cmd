@rem QB64-PE LLVM-MinGW setup script
@rem
@rem This NT command script downloads and extracts the latest copy of LLVM-MinGW binaries from:
@rem https://github.com/mstorsjo/llvm-mingw/releases/latest
@rem
@rem Specifying 32 for argument 1 on a 64-bit system will force a 32-bit LLVM-MinGW setup
@rem
@echo off

rem Enable cmd extensions and exit if not present
setlocal enableextensions
if errorlevel 1 (
    echo.
    echo Error: Command Prompt extensions not available!
    goto end
)

rem Change to the correct drive & path
cd /d "%~dp0"

rem Check if the C++ compiler is there and skip downloading if it exists
if exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Info: LLVM-MinGW detected. Skipping setup.
    goto end
)

rem Create the c_compiler directory that should contain the LLVM-MinGW binaries
mkdir "internal\c\c_compiler"

rem Check if were able to create the directory
if not exist "internal\c\c_compiler\" (
    echo.
    echo Error: Not able to create 'internal\c\c_compiler\'!
    goto end
)

rem Detect architecture
for /f %%a in ('powershell -c "(Get-WmiObject Win32_Processor).Architecture"') do set CPU_ARCH_CODE=%%a

rem Map Windows architecture codes
rem These values are from https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-processor#properties
if "%CPU_ARCH_CODE%"=="0" set CPU_ARCH=X86
if "%CPU_ARCH_CODE%"=="9" set CPU_ARCH=X86
if "%CPU_ARCH_CODE%"=="5" set CPU_ARCH=ARM
if "%CPU_ARCH_CODE%"=="12" set CPU_ARCH=ARM

if "%CPU_ARCH%"=="" (
    echo.
    echo Error: Unknown processor type!
    goto end
)

rem Detect OS architecture
for /f %%a in ('powershell -c "(Get-CimInstance Win32_OperatingSystem).OSArchitecture -match '64'"') do set OS_BITS=64
if not defined OS_BITS set OS_BITS=32

rem Allow forcing 32-bit
if "%~1"=="32" set OS_BITS=32

echo Platform selected: %CPU_ARCH%-%OS_BITS%

rem Query GitHub API for latest LLVM-MinGW release

set LLVM_RELEASE_TAG=
set LLVM_RELEASE_URL=

for /f "usebackq tokens=1" %%a in (`powershell -c "(Invoke-RestMethod 'https://api.github.com/repos/mstorsjo/llvm-mingw/releases/latest').tag_name"`) do set LLVM_RELEASE_TAG=%%a

if "%LLVM_RELEASE_TAG%"=="" (
    for /f "usebackq tokens=*" %%a in (`powershell -NoProfile -Command "$headers = @{ 'Accept' = 'application/vnd.github+json'; 'X-GitHub-Api-Version' = '2022-11-28' }; if ($env:GITHUB_TOKEN) { $headers.Authorization = 'Bearer ' + $env:GITHUB_TOKEN }; try { (Invoke-RestMethod -ErrorAction Stop -Headers $headers 'https://api.github.com/repos/mstorsjo/llvm-mingw/releases/latest').tag_name } catch { '' }" 2^>NUL`) do set LLVM_RELEASE_TAG=%%a
)

if "%LLVM_RELEASE_TAG%"=="" (
    for /f "usebackq tokens=*" %%a in (`curl -fsSIL -o NUL -w "%%{url_effective}" "https://github.com/mstorsjo/llvm-mingw/releases/latest" 2^>NUL`) do set LLVM_RELEASE_URL=%%a
)

if "%LLVM_RELEASE_TAG%"=="" if defined LLVM_RELEASE_URL (
    for /f "usebackq tokens=*" %%a in (`powershell -NoProfile -Command "([System.Uri]'%LLVM_RELEASE_URL%').Segments[-1].TrimEnd('/')" 2^>NUL`) do set LLVM_RELEASE_TAG=%%a
)

if "%LLVM_RELEASE_TAG%"=="" (
    echo.
    echo Error: Unable to detect latest LLVM-MinGW release!
    goto end
)

echo LLVM-MinGW release detected: %LLVM_RELEASE_TAG%

if /I "%CPU_ARCH%"=="ARM" (
    if "%OS_BITS%"=="64" (
        set LLVM_TARGET=aarch64
    ) else (
        set LLVM_TARGET=armv7
    )
) else (
    if "%OS_BITS%"=="64" (
        set LLVM_TARGET=x86_64
    ) else (
        set LLVM_TARGET=i686
    )
)

echo LLVM-MinGW target selected: %LLVM_TARGET%

rem Build directory and URL
set LLVM_DIR_NAME=llvm-mingw-%LLVM_RELEASE_TAG%-ucrt-%LLVM_TARGET%
set LLVM_DOWNLOAD_URL="https://github.com/mstorsjo/llvm-mingw/releases/download/%LLVM_RELEASE_TAG%/%LLVM_DIR_NAME%.zip"

echo Download URL: %LLVM_DOWNLOAD_URL%

set MINGW_TEMP_FILE=temp.zip
if exist "%MINGW_TEMP_FILE%" del "%MINGW_TEMP_FILE%"

rem Download LLVM-MinGW package using curl. curl is available in Windows 10 and above since build 17063
rem https://devblogs.microsoft.com/commandline/tar-and-curl-come-to-windows/
echo Downloading %LLVM_DOWNLOAD_URL%...
curl -L %LLVM_DOWNLOAD_URL% -o "%MINGW_TEMP_FILE%"

rem Extract LLVM-MinGW files using tar. tar is available in Windows 10 and above since build 17063
rem https://devblogs.microsoft.com/commandline/tar-and-curl-come-to-windows/
echo Extracting C++ Compiler...
tar -xvf "%MINGW_TEMP_FILE%"

rem Move the binaries to internal\c\c_compiler\
echo Moving C++ compiler...
for /f %%a in ('dir "%LLVM_DIR_NAME%" /b') do move /y "%LLVM_DIR_NAME%\%%a" "internal\c\c_compiler\"

rem Cleanup downloaded temporary files
echo Cleaning up...
rd "%LLVM_DIR_NAME%"
del "%MINGW_TEMP_FILE%"

:end
endlocal
