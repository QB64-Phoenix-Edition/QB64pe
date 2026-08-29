@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

pushd .

REM Bootstrap is always built as 64-bit because that's the bitness used when
REM generating ./internal/source.
REM
REM Building 32-bit QB64 is done via a cross-compilation using the 64-bit
REM bootstrap compiler.
call setup_mingw.cmd 64

popd

echo Bootstrapping QB64-PE
internal\c\c_compiler\bin\mingw32-make.exe -j8 OS=win BUILD_QB64=y EXE=.\qb64pe_bootstrap.exe
IF ERRORLEVEL 1 exit /b 1

IF "%PLATFORM%"=="x86" (
    REM Replace the 64-bit toolchain with the 32-bit only one, which we'll use
    REM when cross-compiling ./source/qb64pe.bas for 32-bit using the 64-bit bootstrap compiler.
    pushd .
    rmdir /s /q internal\c\c_compiler
    call setup_mingw.cmd 32
    popd
)
