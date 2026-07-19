[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) { throw 'WSL2 is required.' }
$WslRoot = (& wsl.exe wslpath -a ($Root -replace '\\','/')).Trim()
& wsl.exe bash -lc "cd '$WslRoot' && ./scripts/rebuild.sh"
if ($LASTEXITCODE -ne 0) { throw "Rebuild failed with exit code $LASTEXITCODE" }
