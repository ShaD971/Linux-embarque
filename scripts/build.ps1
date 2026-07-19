[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) { throw 'WSL2 is required. Install WSL or build from a Linux machine/container.' }
$WslRoot = (& wsl.exe wslpath -a ($Root -replace '\\','/')).Trim()
if ($LASTEXITCODE -ne 0 -or -not $WslRoot) { throw 'Unable to translate the repository path into WSL.' }
Write-Host 'Delegating Buildroot compilation to WSL2.'
& wsl.exe bash -lc "cd '$WslRoot' && ./scripts/build.sh"
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
