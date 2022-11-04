@rem QB64-PE Windows setup script
@rem
@rem This NT command script downloads and extracts the latest copy of MINGW binaries from:
@rem https://github.com/niXman/mingw-builds-binaries/releases
@rem So, the filenames in 'url' variable should be updated to the latest stable builds when those are available
@rem
@rem Argument 1: If not blank, qb64pe will not be started after compilation
@rem
@echo off

rem Enable cmd extensions and exit if not present
setlocal enableextensions
if errorlevel 1 (
    echo Error: Command Prompt extensions not available!
    goto report_error
)

echo QB64-PE Setup
echo.

rem Change to the correct drive letter
%~d0

rem Change to the correct path
cd %~dp0

rem Check if the C++ compiler is there and skip downloading if it exists
if exist "internal\c\c_compiler\bin\c++.exe" goto skip_mingw_setup

rem Create the c_compiler directory that should contain the MINGW binaries
mkdir internal\c\c_compiler

rem Check if were able to create the directory
if not exist "internal\c\c_compiler\" (
    echo Error: Not able to create 'internal\c\c_compiler\'!
    goto report_error
)

rem Check the processor type and then set the MINGW variable to the correct MINGW arch
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set MINGW=mingw32 || set MINGW=mingw64

rem If the OS is 32-bit then proceed to download right away
if "%MINGW%"=="mingw32" goto set_download_url

rem Check if the user wants to use 32-bit MINGW on a 64-bit system. Default to 64-bit after 60 seconds
choice /t 60 /c 12 /d 1 /m "Do you prefer to download MINGW [1] 64-bit (default) or [2] 32-bit"
if %errorlevel% == 2 set MINGW=mingw32

:set_download_url

rem Set the correct file to download based on processor type
if "%MINGW%"=="mingw32" (
    set url="https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev0/i686-12.2.0-release-win32-sjlj-rt_v10-rev0.7z"
) else (
    set url="https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev0/x86_64-12.2.0-release-win32-seh-rt_v10-rev0.7z"
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

rem Finally check if the C++ compiler is there now
if not exist "internal\c\c_compiler\bin\c++.exe" (
    echo Error: MINGW download / extract failed!
    goto report_error
)

:skip_mingw_setup

rem Run make clean
echo Cleaning...
internal\c\c_compiler\bin\mingw32-make.exe OS=win clean >NUL 2>NUL

rem Now build QB64-PE
echo Building QB64-PE...
internal\c\c_compiler\bin\mingw32-make.exe OS=win BUILD_QB64=y || goto report_error

rem Execute QB64-PE only if there are no parameters
if "%~1"=="" (
    echo.
    echo Launching QB64-PE...
    qb64pe.exe
) else (
    echo.
    pause
)

rem Jump to the end of the script
goto end

rem This is only executed if something on top fails
:report_error
echo.
echo Error compiling QB64-PE.
echo Please review above steps and report to https://github.com/QB64-Phoenix-Edition/QB64pe/issues if you can't get it to work.
endlocal
exit 1

rem The End!
:end
endlocal
exit 0
