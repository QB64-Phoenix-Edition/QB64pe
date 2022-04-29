@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

qb64_bootstrap.exe -x -w source\qb64.bas
IF ERRORLEVEL 1 exit /b 1

cd ..\..

internal\c\c_compiler\bin\mingw32-make.exe OS=win clean

del qb64_bootstrap.exe
