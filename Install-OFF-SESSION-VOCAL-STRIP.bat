@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-OFF-SESSION-VOCAL-STRIP.ps1"
if errorlevel 1 (
  echo.
  echo Installation did not complete. Please send a screenshot of this window.
  pause
  exit /b 1
)
echo.
echo OFF SESSION VOCAL STRIP installation finished.
pause
