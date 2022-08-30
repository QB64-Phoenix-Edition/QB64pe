@echo off
setlocal
echo QB64-PE Setup
echo.

mkdir internal\c\c_compiler
echo.

if exist internal\c\c_compiler\bin\c++.exe goto skipccompsetup
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && goto download32 || goto download64choice

:download32
set MINGW=mingw32.exe
goto downloadfinished

:download64choice
choice /C 12 /M "Do you prefer to download 1)32-bit QB64-PE or 2)64-bit QB64-PE"
if errorlevel == 1 goto download32
set MINGW=mingw64.exe

:downloadfinished

set url="https://www.qb64phoenix.com/qb64_files/%MINGW%"

echo Downloading %url%...
curl %url% -o %MINGW%

echo Extracting %MINGW% as C++ Compiler
@echo off
%MINGW% -y -o"./internal/c/c_compiler/"

:skipccompsetup

echo Cleaning...
internal\c\c_compiler\bin\mingw32-make.exe OS=win clean >NUL 2>NUL

echo Building QB64-PE...
internal\c\c_compiler\bin\mingw32-make.exe OS=win BUILD_QB64=y || goto report_error

echo.
echo Launching 'QB64-PE'
qb64pe

echo.
pause

exit 0

report_error:
echo "Error compiling QB64-PE."
echo "Please review above steps and report to https://github.com/QB64-Phoenix-Edition/QB64pe/issues if you can't get it to work"
exit 1
