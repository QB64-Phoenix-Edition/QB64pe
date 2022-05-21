@echo off
setlocal
echo QB64 Setup
echo.

mkdir internal\c\c_compiler

if exist internal\c\c_compiler\bin\c++.exe goto skipccompsetup
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set MINGW=mingw32.exe || set MINGW=mingw64.exe

set url="https://www.qb64phoenix.com/qb64_files/%MINGW%"

echo Downloading %url%...
curl %url% -o %MINGW%

echo Extracting %MINGW% as C++ Compiler
@echo off
%MINGW% -y -o"./internal/c/c_compiler/"
del %MINGW%

:skipccompsetup

echo Cleaning...
internal\c\c_compiler\bin\mingw32-make.exe OS=win clean >NUL 2>NUL

echo Building QB64...
internal\c\c_compiler\bin\mingw32-make.exe OS=win BUILD_QB64=y || goto report_error

echo.
echo Launching 'QB64'
qb64

echo.
pause

exit 0

report_error:
echo "Error compiling QB64."
echo "Please review above steps and report to https://github.com/QB64-Phoenix-Edition/QB64pe/issues if you can't get it to work"
exit 1
