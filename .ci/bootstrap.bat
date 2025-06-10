@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

pushd .

IF "%PLATFORM%"=="x64" (
    call setup_mingw.cmd 64
) ELSE IF "%PLATFORM%"=="arm64" (
    call setup_mingw.cmd 64
) ELSE (
    call setup_mingw.cmd 32
)

popd

echo Bootstrapping QB64-PE
internal\c\c_compiler\bin\mingw32-make.exe -j8 OS=win BUILD_QB64=y EXE=.\qb64pe_bootstrap.exe
IF ERRORLEVEL 1 exit /b 1
