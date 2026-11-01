@echo off
setlocal
PowerShell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0vm-setup.ps1" %*
if errorlevel 1 (
    echo.
    echo VM setup failed. Read the error above.
    pause
    exit /b 1
)
echo.
echo VM setup completed.
pause
