@rem QB64-PE Windows setup script
@rem
@rem This NT command script calls setup_mingw.cmd which downloads and installs MINGW if required
@rem It then proceeds to build QB64-PE
@rem
@rem ./internal/source is a 64-bit version of QB64-PE so the bootstrap
@rem compiler can only be compiled as 64-bit. Building a 32-bit QB64-PE
@rem requires first building the 64-bit version and then using that to
@rem cross-compile the 32-bit version. The cross-compilation requirement means
@rem that this script only works on 64-bit Windows, 32-bit Windows can use a
@rem 32-bit release copy to build from source.
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
goto check_host

:parse_system_mingw
set USE_SYSTEM_MINGW=1
set MAKE_CMD=mingw32-make
set MAKE_ARGS=USE_SYSTEM_MINGW=y

:check_host

rem The bootstrap compiler is built from ./internal/source, which is generated
rem as 64-bit only. A 32-bit host cannot run the built bootstrap so it cannot
rem build QB64-PE this way.
powershell -NoProfile -Command "if ((Get-WmiObject Win32_OperatingSystem).OSArchitecture -eq '64-bit') { exit 0 } else { exit 1 }" > nul 2> nul
if errorlevel 1 (
    echo.
    echo Error: 32-bit Windows is not supported for building QB64-PE.
    echo.
    echo To build QB64-PE from source, use "./setup_mingw.cmd 32" to install
    echo the 32-bit compiler and then use the latest 32-bit release or CI build of
    echo QB64-PE to build ./source/qb64pe.bas.
    goto report_error
)
echo Detected 64-bit Windows.

if defined USE_SYSTEM_MINGW goto verify_system_mingw

rem Check if the C++ compiler is there and skip MINGW setup if it exists
if exist "internal\c\c_compiler\bin\c++.exe" goto build_qb64pe

rem Choose the target bitness of the QB64-PE binary to build. Default to 64-bit
rem after 60 seconds. Selecting 32-bit triggers a cross-compilation.
set TARGET_BITS=64
choice /t 60 /c 12 /d 1 /m "Build QB64-PE as [1] 64-bit (default) or [2] 32-bit"
if %errorlevel% == 2 set TARGET_BITS=32

if "%TARGET_BITS%" == "32" goto build_cross_32

rem Native 64-bit build: download the 64-bit toolchain and build directly.
pushd .
call setup_mingw.cmd 64
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

:build_cross_32

echo.
echo Building 32-bit QB64-PE by cross-compiling from a 64-bit bootstrap compiler.
echo.

pushd .
call setup_mingw.cmd 64
popd
if not exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Error: 64-bit MINGW setup failed!
    goto report_error
)

echo Cleaning...
%MAKE_CMD% -j%MAKE_JOBS% OS=win clean > nul 2> nul
echo Building 64-bit bootstrap compiler using %MAKE_JOBS% parallel job(s)...
%MAKE_CMD% -j%MAKE_JOBS% OS=win BUILD_QB64=y EXE=.\qb64pe_bootstrap.exe || goto report_error

echo Replacing 64-bit toolchain with 32-bit toolchain...
rmdir /s /q internal\c\c_compiler
pushd .
call setup_mingw.cmd 32
popd
if not exist "internal\c\c_compiler\bin\c++.exe" (
    echo.
    echo Error: 32-bit MINGW setup failed!
    del qb64pe_bootstrap.exe > nul 2> nul
    goto report_error
)

echo Cross-compiling QB64-PE for 32-bit...
qb64pe_bootstrap.exe -x -w -f:TargetBits=32 source\qb64pe.bas
if errorlevel 1 (
    del qb64pe_bootstrap.exe > nul 2> nul
    goto report_error
)

rem Discard the bootstrap compiler, leaving the freshly built qb64pe.exe.
echo Cleaning up 64-bit bootstrap compiler
del qb64pe_bootstrap.exe > nul 2> nul

rem We do a clean here to remove all the 64-bit intermediate files.
rem QB64-PE should do this on the first compile (via seeing the
rem `./internal/c/.qb64_target_bits` from the bootstrap) but it's faster to do
rem it here.
%MAKE_CMD% -j%MAKE_JOBS% OS=win clean > nul 2> nul

echo.
echo Build complete.
goto end

:usage
echo.
echo Usage:
echo   setup_win.cmd [/s ^| -s]
echo.
echo Default: Uses bundled MinGW in internal\c\c_compiler and bootstraps it if needed.
echo   Prompts to build a 64-bit (default) or 32-bit QB64-PE. The 32-bit build is
echo   produced by cross-compiling from a 64-bit bootstrap compiler.
echo Switch /s or -s: Uses system-installed MinGW from PATH and passes USE_SYSTEM_MINGW=y
echo   (64-bit build only).
echo A 32-bit host is not supported; QB64-PE can only be built from a 64-bit host.
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
