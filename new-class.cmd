@echo off
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\New-Class.ps1" %*
exit /b %errorlevel%
