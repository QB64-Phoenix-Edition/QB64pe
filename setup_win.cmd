@rem QB64-PE Windows setup script
@rem
@rem This NT command script calls setup_mingw.cmd which downloads and installs MINGW if required.
@rem It then proceeds to build QB64-PE.
@rem
@rem Optional switches:
@rem   /s or -s       Build using system-installed MinGW from PATH (uses USE_SYSTEM_MINGW=y)
@rem   -help or /?    Show usage
@rem
@rem Windows 7 compatibility:
@rem - Does not use PowerShell to detect OS bitness.
@rem - setup_mingw.cmd first uses normal curl/Git for Windows when available.
@rem - If only GitHub Desktop is present, setup_mingw.cmd can use its Electron/Node
@rem   runtime for HTTPS and Windows Shell.Application for ZIP extraction.
@rem
@echo off

rem Enable cmd extensions and exit if not present
setlocal EnableExtensions
set "SETUP_RESULT=0"
if errorlevel 1 (
    echo.
    echo Error: Command Prompt extensions not available!
    goto report_error
)

echo QB64-PE Setup
echo.

rem Change to the correct drive & path
cd /d "%~dp0"

set "USE_SYSTEM_MINGW="
set "MAKE_CMD=internal\c\c_compiler\bin\mingw32-make.exe"
set "MAKE_ARGS="
set "MAKE_JOBS=%NUMBER_OF_PROCESSORS%"
if not defined MAKE_JOBS set "MAKE_JOBS=1"
set /a MAKE_JOBS=MAKE_JOBS+0 >nul 2>nul
if errorlevel 1 set "MAKE_JOBS=1"
if %MAKE_JOBS% lss 1 set "MAKE_JOBS=1"

if /i "%~1"=="/s" goto parse_system_mingw
if /i "%~1"=="-s" goto parse_system_mingw
if /i "%~1"=="/?" goto usage
if /i "%~1"=="-help" goto usage
if not "%~1"=="" (
    echo.
    echo Error: Unknown option %~1
    goto usage
)
goto setup_compiler

:parse_system_mingw
set "USE_SYSTEM_MINGW=1"
set "MAKE_CMD=mingw32-make"
set "MAKE_ARGS=USE_SYSTEM_MINGW=y"
goto setup_compiler

:setup_compiler
if defined USE_SYSTEM_MINGW goto verify_system_mingw

rem Check if the C++ compiler is there and skip MinGW setup if it exists
if exist "internal\c\c_compiler\bin\c++.exe" goto build_qb64pe

rem Detect native OS bitness without PowerShell. PROCESSOR_ARCHITEW6432 is
rem defined when a 32-bit process runs under WOW64 on 64-bit Windows.
set "BITS=32"
if /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "BITS=64"
if /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "BITS=64"
if defined PROCESSOR_ARCHITEW6432 set "BITS=64"

echo Detected %BITS%-bit Windows.

rem If the OS is 32-bit then proceed to download right away
if "%BITS%"=="32" goto setup_mingw

rem Check if the user wants to use 32-bit MinGW on a 64-bit system.
rem Default to 64-bit after 60 seconds.
choice /t 60 /c 12 /d 1 /m "Do you prefer to download MinGW [1] 64-bit (default) or [2] 32-bit"
if "%ERRORLEVEL%"=="2" set "BITS=32"

:setup_mingw
rem Call the MinGW setup script using the BITS variable
pushd .
call setup_mingw.cmd %BITS%
set "MINGW_SETUP_RESULT=%ERRORLEVEL%"
popd

rem Finally check if the C++ compiler is there now
if not "%MINGW_SETUP_RESULT%"=="0" (
    echo.
    echo Error: MINGW setup script reported failure!
    goto report_error
)

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
"%MAKE_CMD%" -j%MAKE_JOBS% OS=win %MAKE_ARGS% clean >nul 2>nul

rem Now build QB64-PE
echo Building QB64-PE using %MAKE_JOBS% parallel job(s)...
"%MAKE_CMD%" -j%MAKE_JOBS% OS=win %MAKE_ARGS% BUILD_QB64=y || goto report_error

echo.
echo Build complete.
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

:report_error
set "SETUP_RESULT=1"
echo.
echo Error compiling QB64-PE.
echo Please review above steps and report to https://github.com/QB64-Phoenix-Edition/QB64pe/issues if you can't get it to work.

:end
endlocal & exit /b %SETUP_RESULT%
