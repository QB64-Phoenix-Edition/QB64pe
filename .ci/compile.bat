@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM qb64pe_bootstrap.exe is always 64-bit (see bootstrap.bat). For PLATFORM=x86
REM cross-compile the real qb64pe.exe down to 32-bit instead.
SET TARGETBITS_FLAG=
IF "%PLATFORM%"=="x86" SET TARGETBITS_FLAG=-f:TargetBits=32

qb64pe_bootstrap.exe -x -w %TARGETBITS_FLAG% source\qb64pe.bas
IF ERRORLEVEL 1 exit /b 1

del qb64pe_bootstrap.exe
del /q /s internal\source\*
move internal\temp\* internal\source\

REM Build libqb test executables
internal\c\c_compiler\bin\mingw32-make.exe -j8 OS=win build-tests

internal\c\c_compiler\bin\mingw32-make.exe OS=win clean

