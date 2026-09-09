@rem QB64-PE LLVM-MinGW setup launcher
@rem
@rem All setup logic lives in setup_mingw.ps1 and intentionally targets
@rem Windows PowerShell 2.0 for Windows 7 compatibility.
@rem
@rem Specifying 32 for argument 1 forces a 32-bit LLVM-MinGW setup.
@echo off
setlocal enableextensions

set "QB64PE_SETUP_PS=%~dp0setup_mingw.ps1"

if not exist "%QB64PE_SETUP_PS%" (
    echo.
    echo Error: setup_mingw.ps1 was not found next to setup_mingw.cmd!
    endlocal
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%QB64PE_SETUP_PS%" %*
set "QB64PE_SETUP_RESULT=%ERRORLEVEL%"

endlocal & exit /b %QB64PE_SETUP_RESULT%
