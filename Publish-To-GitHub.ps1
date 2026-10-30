$ErrorActionPreference = 'Stop'
$repo = 'sleeepboii123-bot/OFF-SESSION-VOCAL-STRIP'
$source = $PSScriptRoot
$work = Join-Path $env:TEMP 'OFF-SESSION-VOCAL-STRIP-PUBLISH'

function Refresh-Path {
    $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $user = [Environment]::GetEnvironmentVariable('Path', 'User')
    $env:Path = $machine + ';' + $user
    $extra = @(
        (Join-Path $env:ProgramFiles 'Git\cmd'),
        (Join-Path $env:ProgramFiles 'GitHub CLI'),
        (Join-Path $env:LOCALAPPDATA 'Programs\GitHub CLI'),
        (Join-Path $env:LOCALAPPDATA 'Programs\Git\cmd')
    )
    foreach ($path in $extra) { if ((Test-Path $path) -and ($env:Path -notlike "*$path*")) { $env:Path += ';' + $path } }
}
function Require-Command([string]$name) {
    if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
        throw "$name was not found. Install Git for Windows and GitHub CLI once, then run this helper again."
    }
}

Write-Host 'OFF SESSION VOCAL STRIP - Publish source to GitHub' -ForegroundColor Magenta
Write-Host "Target: $repo" -ForegroundColor Cyan
Write-Host 'This creates a separate repository. Your GUMZYY CALL FX project is not modified.'
Refresh-Path
Require-Command 'git'
Require-Command 'gh'

& gh auth status --hostname github.com *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host 'A browser window will open to authorize GitHub CLI.' -ForegroundColor Yellow
    & gh auth login --hostname github.com --git-protocol https --web
    if ($LASTEXITCODE -ne 0) { throw 'GitHub authentication did not complete.' }
}
& gh auth setup-git
if ($LASTEXITCODE -ne 0) { throw 'Could not configure Git authentication.' }

$previousPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & gh repo view $repo *> $null
    $repoViewExitCode = $LASTEXITCODE
} finally { $ErrorActionPreference = $previousPreference }

if ($repoViewExitCode -ne 0) {
    Write-Host 'Creating the new project repository...' -ForegroundColor Yellow
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & gh repo create $repo --public --description 'OFF SESSION VOCAL STRIP - modular low-latency vocal channel strip VST3 by NYXV x OFF SESSION'
        $repoCreateExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($repoCreateExitCode -ne 0) { throw 'Could not create the repository. Check the GitHub account and repository name.' }
}

if (Test-Path $work) { Remove-Item -Recurse -Force $work }
Write-Host 'Cloning the project repository...' -ForegroundColor Yellow
& gh repo clone $repo $work
if ($LASTEXITCODE -ne 0) { throw 'Could not clone the new repository.' }

Write-Host 'Copying the complete plugin project...' -ForegroundColor Yellow
Get-ChildItem -LiteralPath $source -Force | Where-Object { $_.Name -notin @('.git', 'build', 'release') } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $work -Recurse -Force
}

Push-Location $work
try {
    $userJson = gh api user
    if ($LASTEXITCODE -ne 0) { throw 'Could not read the authenticated GitHub account.' }
    $user = $userJson | ConvertFrom-Json
    & git config user.name $user.login
    & git config user.email ("{0}+{1}@users.noreply.github.com" -f $user.id, $user.login)
    & git add -A
    if ($LASTEXITCODE -ne 0) { throw 'Git could not stage the project files.' }
    & git diff --cached --quiet
    if ($LASTEXITCODE -eq 0) {
        Write-Host 'No source changes to publish.' -ForegroundColor Green
    } else {
        & git commit -m 'Create OFF SESSION VOCAL STRIP v0.1.0'
        if ($LASTEXITCODE -ne 0) { throw 'Git commit failed.' }
        & git push origin HEAD:main
        if ($LASTEXITCODE -ne 0) { throw 'Git push failed.' }
        Write-Host 'Source pushed successfully. The Windows build workflow should start automatically.' -ForegroundColor Green
    }
} finally { Pop-Location }

$actions = "https://github.com/$repo/actions"
Write-Host ''
Write-Host "Open the Windows build here: $actions" -ForegroundColor Cyan
Start-Process $actions
Read-Host 'Press Enter to close'
