@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

qb64pe_bootstrap.exe -x -w source\qb64pe.bas
IF ERRORLEVEL 1 exit /b 1

del qb64pe_bootstrap.exe
del /q /s internal\source\*
move internal\temp\* internal\source\

REM Build libqb test executables
internal\c\c_compiler\bin\mingw32-make.exe -j8 OS=win build-tests

internal\c\c_compiler\bin\mingw32-make.exe OS=win clean

