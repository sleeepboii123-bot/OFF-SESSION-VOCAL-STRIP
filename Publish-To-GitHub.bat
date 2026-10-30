@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Publish-To-GitHub.ps1"
if errorlevel 1 (
  echo.
  echo Publishing did not complete. Please send a screenshot of this window.
  pause
  exit /b 1
)
echo.
echo Source published. Open the GitHub Actions page shown by the script.
pause
