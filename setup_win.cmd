@rem QB64-PE Windows setup script
@rem
@rem This NT command script calls setup_mingw.cmd which downloads and installs MINGW if required
@rem It then proceeds to build QB64-PE
@rem
@rem Optional switches:
@rem   /s or -s       Build using system-installed MinGW from PATH (uses USE_SYSTEM_MINGW=y)
@rem   -help          Show usage
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

set USE_SYSTEM_MINGW=
set MAKE_CMD=internal\c\c_compiler\bin\mingw32-make.exe
set MAKE_ARGS=
set /a MAKE_JOBS=%NUMBER_OF_PROCESSORS% > nul 2> nul
if %errorlevel% neq 0 set MAKE_JOBS=1
if %MAKE_JOBS% lss 1 set MAKE_JOBS=1

if /i "%~1"=="/s" goto parse_system_mingw
if /i "%~1"=="-s" goto parse_system_mingw
if /i "%~1"=="/?" goto usage
if not "%~1"=="" (
    echo.
    echo Error: Unknown option %~1
    goto usage
)
goto setup_compiler

:parse_system_mingw
set USE_SYSTEM_MINGW=1
set MAKE_CMD=mingw32-make
set MAKE_ARGS=USE_SYSTEM_MINGW=y
goto setup_compiler

:setup_compiler

if defined USE_SYSTEM_MINGW goto verify_system_mingw

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

goto build_qb64pe

:verify_system_mingw
where /q mingw32-make || (
    echo.
    echo Error: mingw32-make was not found in PATH.
    echo Install MinGW and ensure mingw32-make, gcc, and g++ are available in PATH,
    echo or run setup_win.cmd without /s.
    goto report_error
)

where /q gcc || (
    echo.
    echo Error: gcc was not found in PATH.
    echo Install MinGW and ensure gcc and g++ are available in PATH.
    goto report_error
)

where /q g++ || (
    echo.
    echo Error: g++ was not found in PATH.
    echo Install MinGW and ensure gcc and g++ are available in PATH.
    goto report_error
)

:build_qb64pe

rem Run make clean
echo Cleaning...
%MAKE_CMD% -j%MAKE_JOBS% OS=win %MAKE_ARGS% clean > nul 2> nul

rem Now build QB64-PE
echo Building QB64-PE using %MAKE_JOBS% parallel job(s)...
%MAKE_CMD% -j%MAKE_JOBS% OS=win %MAKE_ARGS% BUILD_QB64=y || goto report_error

echo.
echo Build complete.

rem Jump to the end of the script
goto end

:usage
echo.
echo Usage:
echo   setup_win.cmd [/s ^| -s]
echo.
echo Default: Uses bundled MinGW in internal\c\c_compiler and bootstraps it if needed.
echo Switch /s or -s: Uses system-installed MinGW from PATH and passes USE_SYSTEM_MINGW=y.
echo Parallel build jobs are set automatically from NUMBER_OF_PROCESSORS.
goto end

rem This is only executed if something on top fails
:report_error
echo.
echo Error compiling QB64-PE.
echo Please review above steps and report to https://github.com/QB64-Phoenix-Edition/QB64pe/issues if you can't get it to work.

rem The End!
:end
endlocal
