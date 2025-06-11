@rem QB64-PE MinGW setup script
@rem
@rem This NT command script downloads and extracts the latest copy of LLVM MinGW binaries from:
@rem https://github.com/mstorsjo/llvm-mingw/releases
@rem
@rem Specifying 32 for argument 1 on a 64-bit system will force a 32-bit MinGW setup
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
cd /d %~dp0

rem Check if the C++ compiler is there and skip downloading if it exists
if exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Info: MinGW detected. Skipping setup.
    goto end
)

rem Create the c_compiler directory that should contain the MinGW binaries
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
powershell -c "(Get-WmiObject Win32_Processor).Architecture" | find "0" > nul && set ARCH=X86
powershell -c "(Get-WmiObject Win32_Processor).Architecture" | find "5" > nul && set ARCH=ARM
powershell -c "(Get-WmiObject Win32_Processor).Architecture" | find "9" > nul && set ARCH=X86
powershell -c "(Get-WmiObject Win32_Processor).Architecture" | find "12" > nul && set ARCH=ARM

rem Check if this is an alien processor
if "%ARCH%" == "" (
  echo Error: Unknown processor type!
  goto end
)

rem Check the processor type and then set the BITS variable
powershell -c "(Get-WmiObject Win32_OperatingSystem).OsArchitecture" | find /i "64-bit" > nul && set BITS=64 || set BITS=32

echo %ARCH%-%BITS% platform detected.

rem Check if "32" was passed as an argument and if so set BITS to 32
if "%~1" == "32" set BITS=32

echo %ARCH%-%BITS% platform selected.

rem Set some critical variables before we move to the actual setup part
rem The filenames in 'URL' variable should be updated to the latest stable builds when those are available
rem MINGW_DIR is actually the internal directory name inside the zip file
rem It needs to be updated whenever the toolchains are updated
if "%ARCH%" == "ARM" (
    if %BITS% == 64 (
        set URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20250514/llvm-mingw-20250514-ucrt-aarch64.zip"
        set MINGW_DIR=llvm-mingw-20250514-ucrt-aarch64
    ) else (
        set URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20250514/llvm-mingw-20250514-ucrt-armv7.zip"
        set MINGW_DIR=llvm-mingw-20250514-ucrt-armv7
    )
) else (
    if %BITS% == 64 (
        set URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20250514/llvm-mingw-20250514-ucrt-x86_64.zip"
        set MINGW_DIR=llvm-mingw-20250514-ucrt-x86_64
    ) else (
        set URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20250514/llvm-mingw-20250514-ucrt-i686.zip"
        set MINGW_DIR=llvm-mingw-20250514-ucrt-i686
    )
)

rem Download LLVM-MinGW package using curl. curl is available in Windows 10 and above since build 17063
rem https://devblogs.microsoft.com/commandline/tar-and-curl-come-to-windows/
set MINGW_TEMP_FILE=temp.zip
if exist %MINGW_TEMP_FILE% del %MINGW_TEMP_FILE%
echo Downloading %URL%...
curl -L %URL% -o %MINGW_TEMP_FILE%

rem Extract LLVM-MinGW files using tar. tar is available in Windows 10 and above since build 17063
rem https://devblogs.microsoft.com/commandline/tar-and-curl-come-to-windows/
echo Extracting C++ Compiler...
tar -xvf %MINGW_TEMP_FILE%

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
