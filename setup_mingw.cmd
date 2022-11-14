@rem QB64-PE MINGW setup script
@rem
@rem This NT command script downloads and extracts the latest copy of MINGW binaries from:
@rem https://github.com/niXman/mingw-builds-binaries/releases
@rem So, the filenames in 'url' variable should be updated to the latest stable builds when those are available
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

rem Check the processor type and then set the MINGW variable to the correct MINGW arch
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set MINGW=mingw32 || set MINGW=mingw64

rem If the OS is 32-bit then proceed to download right away
rem Else check if 32 was passed as a parameter. If so, download 32-bit MINGW on a 64-bit system
if "%MINGW%"=="mingw64" if "%~1"=="32" set MINGW=mingw32

rem Set the correct file to download based on processor type
if "%MINGW%"=="mingw32" (
    set url="https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev1/i686-12.2.0-release-win32-dwarf-rt_v10-rev1.7z"
) else (
    set url="https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev1/x86_64-12.2.0-release-win32-seh-rt_v10-rev1.7z"
)

rem Download 7zr.exe. We'll need this to extract the MINGW archive
echo Downloading 7zr.exe...
curl -L https://www.7-zip.org/a/7zr.exe -o 7zr.exe

rem Download the MINGW archive
echo Downloading %url%...
curl -L %url% -o temp.7z

rem Extract the MINGW binaries
echo Extracting C++ Compiler...
7zr.exe x temp.7z -y

rem Move the binaries to internal\c\c_compiler\
echo Moving C++ compiler...
for /f %%a in ('dir %MINGW% /b') do move /y "%MINGW%\%%a" internal\c\c_compiler\

rem Cleanup downloaded temporary files
echo Cleaning up...
rd %MINGW%
del 7zr.exe
del temp.7z

rem The End!
:end
endlocal
