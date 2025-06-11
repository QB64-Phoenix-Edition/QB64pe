@rem QB64-PE Windows setup script
@rem
@rem This NT command script calls setup_mingw.cmd which downloads and installs MINGW if required
@rem It then proceeds to build QB64-PE
@rem
@rem If argument 1 is not blank, then qb64pe will not be started after compilation
@rem
@echo off

rem Enable cmd extensions and exit if not present
setlocal enableextensions
if errorlevel 1 (
    echo.
    echo Error: Command Prompt extensions not available!
    goto report_error
)

echo QB64-PE Setup
echo.

rem Change to the correct drive & path
cd /d %~dp0

rem Check if the C++ compiler is there and skip MINGW setup if it exists
if exist "internal\c\c_compiler\bin\c++.exe" goto build_qb64pe

rem Check the processor type and then set the BITS variable
powershell -c "(Get-WmiObject Win32_OperatingSystem).OsArchitecture" | find /i "64-bit" > nul && set BITS=64 || set BITS=32

rem If the OS is 32-bit then proceed to download right away
if %BITS% == 32 goto setup_mingw

rem Check if the user wants to use 32-bit MINGW on a 64-bit system. Default to 64-bit after 60 seconds
choice /t 60 /c 12 /d 1 /m "Do you prefer to download MinGW [1] 64-bit (default) or [2] 32-bit"
if %errorlevel% == 2 set BITS=32

:setup_mingw

rem Call the MINGW setup script using the BITS variable
pushd .
call setup_mingw.cmd %BITS%
popd

rem Finally check if the C++ compiler is there now
if not exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Error: MINGW setup failed!
    goto report_error
)

:build_qb64pe

rem Run make clean
echo Cleaning...
internal\c\c_compiler\bin\mingw32-make.exe OS=win clean > nul 2> nul

rem Now build QB64-PE
echo Building QB64-PE...
internal\c\c_compiler\bin\mingw32-make.exe OS=win BUILD_QB64=y || goto report_error

rem Execute QB64-PE only if there are no parameters
if "%~1" == "" (
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

rem The End!
:end
endlocal
