$ErrorActionPreference = 'Stop'

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host 'Requesting administrator permission to install the VST3...' -ForegroundColor Yellow
    $args = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"")
    $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $args -Verb RunAs -Wait -PassThru
    exit $process.ExitCode
}

$plugin = Get-ChildItem -LiteralPath $PSScriptRoot -Directory -Recurse -Filter 'OFF SESSION VOCAL STRIP.vst3' | Select-Object -First 1
if (-not $plugin) {
    throw 'OFF SESSION VOCAL STRIP.vst3 was not found. Extract the complete download ZIP before running the installer.'
}

$targetRoot = Join-Path $env:CommonProgramFiles 'VST3'
New-Item -ItemType Directory -Force -Path $targetRoot | Out-Null
$destination = Join-Path $targetRoot $plugin.Name

Write-Host ''
Write-Host 'OFF SESSION VOCAL STRIP - VST3 installer' -ForegroundColor Magenta
Write-Host "Source:      $($plugin.FullName)"
Write-Host "Destination: $destination"

if (Test-Path -LiteralPath $destination) {
    Remove-Item -LiteralPath $destination -Recurse -Force
}
Copy-Item -LiteralPath $plugin.FullName -Destination $targetRoot -Recurse -Force

Write-Host ''
Write-Host 'Installation successful.' -ForegroundColor Green
Write-Host 'Restart Ableton Live, then rescan VST3 plug-ins if it is already open.' -ForegroundColor Cyan
Read-Host 'Press Enter to close'
