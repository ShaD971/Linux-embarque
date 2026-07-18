[CmdletBinding()]
param(
  [string]$Version = (Get-Content (Join-Path $PSScriptRoot '..\buildroot.version') -Raw).Trim(),
  [switch]$Force
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Buildroot = Join-Path $Root 'buildroot'
$Downloads = Join-Path $Root 'dl'
$Archive = Join-Path $Downloads "buildroot-$Version.tar.xz"
$Url = "https://buildroot.org/downloads/buildroot-$Version.tar.xz"

Write-Host 'Buildroot compilation requires WSL2, Linux, or a Linux container.'
if ((Test-Path (Join-Path $Buildroot 'Makefile')) -and -not $Force) {
  $answer = Read-Host "Buildroot already exists in $Buildroot. Replace it? [y/N]"
  if ($answer -notmatch '^[Yy]$') { return }
}
if (Test-Path $Buildroot) { Remove-Item -LiteralPath $Buildroot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Downloads, (Join-Path $Root 'output'), (Join-Path $Root 'logs') | Out-Null
if (-not (Test-Path $Archive) -or (Get-Item $Archive).Length -eq 0) {
  try { Invoke-WebRequest -Uri $Url -OutFile $Archive -UseBasicParsing }
  catch { throw "Download failed from $Url. $($_.Exception.Message)" }
}
if (-not (Test-Path $Archive) -or (Get-Item $Archive).Length -eq 0) { throw "Missing or empty archive: $Archive" }
$Temporary = Join-Path $Root ".buildroot-extract-$Version"
if (Test-Path $Temporary) { Remove-Item -LiteralPath $Temporary -Recurse -Force }
New-Item -ItemType Directory -Path $Temporary | Out-Null
try {
  & tar -xJf $Archive -C $Temporary
  if ($LASTEXITCODE -ne 0) { throw 'tar extraction failed' }
  Move-Item -LiteralPath (Join-Path $Temporary "buildroot-$Version") -Destination $Buildroot
} finally {
  if (Test-Path $Temporary) { Remove-Item -LiteralPath $Temporary -Recurse -Force }
}
$line = Select-String -Path (Join-Path $Buildroot 'Makefile') -Pattern '^export BR2_VERSION :=' | Select-Object -First 1
Write-Host "Installed $($line.Line) in $Buildroot"
Write-Host 'Next: .\scripts\build.ps1'
Write-Host "Submodule alternative (display only): git submodule add -b $Version https://gitlab.com/buildroot.org/buildroot.git buildroot"
