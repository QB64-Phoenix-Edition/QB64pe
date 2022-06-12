@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

mkdir internal\c\c_compiler

set MINGW=mingw32.exe
IF "%PLATFORM%"=="x64" set MINGW=mingw64.exe

set url="https://www.qb64phoenix.com/qb64_files/%MINGW%"

echo Downloading %url%...
curl %url% -o %MINGW%

echo Extracting %MINGW% as C++ Compiler
@echo off
%MINGW% -y -o"./internal/c/c_compiler/"

echo Bootstrapping QB64
internal\c\c_compiler\bin\mingw32-make.exe -j8 OS=win BUILD_QB64=y EXE=.\qb64_bootstrap.exe
IF ERRORLEVEL 1 exit /b 1

