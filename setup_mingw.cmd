@rem QB64-PE MINGW setup script
@rem
@rem This NT command script downloads and extracts the latest copy of MINGW binaries from:
@rem https://github.com/niXman/mingw-builds-binaries/releases
@rem So, the filenames in 'URL' variable should be updated to the latest stable builds when those are available
@rem
@rem Specifying 32 for argument 1 on a 64-bit system will force a 32-bit MINGW setup
@rem
@echo off

rem Enable cmd extensions and exit if not present
setlocal enableextensions
if errorlevel 1 (
    echo.
    echo Error: Command Prompt extensions not available!
    goto end
)

rem Change to the correct drive letter
%~d0

rem Change to the correct path
cd %~dp0

rem Check if the C++ compiler is there and skip downloading if it exists
if exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Info: MINGW detected. Skipping setup.
    goto end
)

rem Create the c_compiler directory that should contain the MINGW binaries
mkdir internal\c\c_compiler

rem Check if were able to create the directory
if not exist "internal\c\c_compiler\" (
    echo.
    echo Error: Not able to create 'internal\c\c_compiler\'!
    goto end
)

rem Check the processor type and then set the ARCH variable to the processor type
rem These values are from https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-processor#properties
set "ARCH="
wmic cpu get architecture | find "0" > nul && set ARCH=X86
wmic cpu get architecture | find "5" > nul && set ARCH=ARM
wmic cpu get architecture | find "9" > nul && set ARCH=X86
wmic cpu get architecture | find "12" > nul && set ARCH=ARM

rem Check if this is an alien processor
if "%ARCH%" == "" (
  echo Error: Unknown processor type!
  goto end
)

rem Check the processor type and then set the BITS variable
wmic os get osarchitecture | find /i "64-bit" > nul && set BITS=64 || set BITS=32

echo %ARCH%-%BITS% platform detected.

rem Check if "32" was passed as an argument and if so set BITS to 32
if "%~1" == "32" set BITS=32

echo %ARCH%-%BITS% platform selected.

rem Set some critical variables before we move to the actual setup part
rem MINGW_DIR is actually the internal directory name inside the zip / 7z file
rem It needs to be updated whenever the toolchains are updated
if "%ARCH%" == "ARM" (
    if %BITS% == 64 (
        set URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20240320/llvm-mingw-20240320-ucrt-aarch64.zip"
        set MINGW_DIR=llvm-mingw-20240320-ucrt-aarch64
    ) else (
        set URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20240320/llvm-mingw-20240320-ucrt-armv7.zip"
        set MINGW_DIR=llvm-mingw-20240320-ucrt-armv7
    )
    set MINGW_TEMP_FILE=temp.zip
) else (
    if %BITS% == 64 (
        set URL="https://github.com/niXman/mingw-builds-binaries/releases/download/13.2.0-rt_v11-rev0/x86_64-13.2.0-release-win32-seh-msvcrt-rt_v11-rev0.7z"
        set MINGW_DIR=mingw64
    ) else (
        set URL="https://github.com/niXman/mingw-builds-binaries/releases/download/13.2.0-rt_v11-rev0/i686-13.2.0-release-win32-dwarf-msvcrt-rt_v11-rev0.7z"
        set MINGW_DIR=mingw32
    )    
    set MINGW_TEMP_FILE=temp.7z
)

rem Download MingW files
if exist %MINGW_TEMP_FILE% del %MINGW_TEMP_FILE%
echo Downloading %URL%...
curl -L %URL% -o %MINGW_TEMP_FILE%

rem Extract MingW files
if "%ARCH%" == "ARM" (
    rem Use tar to extract the zip file on Windows on ARM. tar is available in Windows since build 17063
    rem And this includes all ARM64 Windows versions
    echo Extracting C++ Compiler...
    tar -xvf %MINGW_TEMP_FILE%
) else (
    rem Download 7zr.exe. We'll need this to extract the MINGW archive
    echo Downloading 7zr.exe...
    curl -L https://www.7-zip.org/a/7zr.exe -o 7zr.exe

    rem Extract the MINGW binaries
    echo Extracting C++ Compiler...
    7zr.exe x %MINGW_TEMP_FILE% -y
    del 7zr.exe
)

rem Move the binaries to internal\c\c_compiler\
echo Moving C++ compiler...
for /f %%a in ('dir %MINGW_DIR% /b') do move /y "%MINGW_DIR%\%%a" internal\c\c_compiler\

rem Cleanup downloaded temporary files
echo Cleaning up...
rd %MINGW_DIR%
del %MINGW_TEMP_FILE%

rem The End!
:end
endlocal
