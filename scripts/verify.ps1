[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ReportDir = Join-Path $Root 'build'
$Report = Join-Path $ReportDir 'modernization_verification_report.txt'
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$Results = [System.Collections.Generic.List[string]]::new()
$Failures = 0

function Add-Check([string]$Name, [bool]$Passed, [string]$Detail) {
  $script:Failures += [int](-not $Passed)
  $status = if ($Passed) { 'PASS' } else { 'FAIL' }
  $Results.Add("[$status] $Name - $Detail")
}

Push-Location $Root
try {
  $git = Get-Command git -ErrorAction SilentlyContinue
  Add-Check 'Git available' ([bool]$git) $(if ($git) { $git.Source } else { 'not found' })
  $wsl = Get-Command wsl.exe -ErrorAction SilentlyContinue
  Add-Check 'WSL available' ([bool]$wsl) $(if ($wsl) { 'WSL command found' } else { 'optional for checks; required for Windows builds' })

  $required = @(
    'buildroot.version', 'external\external.desc', 'external\Config.in', 'external\external.mk',
    'external\configs\linux_embarque_defconfig', 'external\board\linux-embarque\linux.config',
    'external\board\linux-embarque\cmdline.txt', 'external\board\linux-embarque\config_4_64bit.txt',
    'external\board\linux-embarque\genimage.cfg',
    'scripts\setup.sh', 'scripts\build.sh', 'scripts\rebuild.sh', 'scripts\clean.sh',
    'scripts\setup.ps1', 'scripts\build.ps1', '.github\workflows\build.yml'
  )
  foreach ($path in $required) { Add-Check "Required file $path" (Test-Path $path) $path }

  $desc = Get-Content 'external\external.desc' -Raw
  Add-Check 'BR2_EXTERNAL name' ($desc -match '(?m)^name: LINUX_EMBARQUE$') 'stable external name'
  $defconfig = Get-Content 'external\configs\linux_embarque_defconfig' -Raw
  Add-Check 'Target architecture' ($defconfig -match '(?m)^BR2_aarch64=y$' -and $defconfig -match '(?m)^BR2_cortex_a72=y$') 'Raspberry Pi 4 Cortex-A72 64-bit'
  Add-Check 'Raspberry Pi kernel pin' ($defconfig -match '576cc10e1ed50a9eacffc7a05c796051d7343ea4') 'Buildroot 2025.02.16 Raspberry Pi kernel'
  Add-Check 'Raspberry Pi 4 DTB' ($defconfig -match 'broadcom/bcm2711-rpi-4-b') 'BCM2711 Model B'
  Add-Check 'Raspberry Pi 4 firmware' ($defconfig -match '(?m)^BR2_PACKAGE_RPI_FIRMWARE_VARIANT_PI4=y$') 'start4/fixup4 firmware'
  Add-Check 'Raspberry Pi Wi-Fi firmware' ($defconfig -match '(?m)^BR2_PACKAGE_BRCMFMAC_SDIO_FIRMWARE_RPI_WIFI=y$') 'BCM43455 firmware'
  Add-Check 'No empty root password' ($defconfig -match '(?m)^# BR2_TARGET_ENABLE_ROOT_LOGIN is not set$') 'root login disabled by default'
  $cmdline = Get-Content 'external\board\linux-embarque\cmdline.txt' -Raw
  Add-Check 'Raspberry Pi kernel command line' ($cmdline -match 'root=/dev/mmcblk0p2' -and $cmdline -match 'rootwait') 'root filesystem and rootwait configured'
  $firmwareConfig = Get-Content 'external\board\linux-embarque\config_4_64bit.txt' -Raw
  Add-Check '64-bit firmware boot' ($firmwareConfig -match '(?m)^arm_64bit=1$' -and $firmwareConfig -match '(?m)^kernel=Image$') 'AArch64 Image selected'
  $genimageConfig = Get-Content 'external\board\linux-embarque\genimage.cfg' -Raw
  Add-Check 'Pi 4 SD image contents' ($genimageConfig -match 'bcm2711-rpi-4-b\.dtb' -and $genimageConfig -match 'start4\.elf' -and $genimageConfig -notmatch 'bootcode\.bin') 'Pi 4 EEPROM boot layout'
  $workflow = Get-Content '.github\workflows\build.yml' -Raw
  Add-Check 'CI parallelism bounded' ($workflow -match '(?m)^\s+BUILD_JOBS:\s+2\s*$') 'BUILD_JOBS=2'
  Add-Check 'CI diagnostics always uploaded' ($workflow -match '(?ms)- name: Upload build diagnostics.*?if: always\(\)') 'logs and stamps retained on failure'

  $lfFiles = @(
    Get-ChildItem 'scripts','external' -Recurse -File -Filter '*.sh'
    Get-Item 'external\board\linux-embarque\cmdline.txt'
    Get-Item 'external\board\linux-embarque\config_4_64bit.txt'
    Get-Item 'external\board\linux-embarque\genimage.cfg'
  )
  foreach ($file in $lfFiles) {
    $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
    $hasCrLf = $false
    for ($i = 0; $i -lt $bytes.Length - 1; $i++) { if ($bytes[$i] -eq 13 -and $bytes[$i + 1] -eq 10) { $hasCrLf = $true; break } }
    Add-Check "LF endings $($file.Name)" (-not $hasCrLf) $file.FullName
  }

  $secretPattern = '(?i)(BEGIN (RSA|OPENSSH|EC) PRIVATE KEY|AKIA[0-9A-Z]{16}|sk-[A-Za-z0-9_-]{20,}|password\s*[:=]\s*[^\s#]+)'
  $scanFiles = Get-ChildItem 'external','scripts','.github' -Recurse -File | Where-Object { $_.Length -lt 1MB }
  $secretHits = $scanFiles | Select-String -Pattern $secretPattern -ErrorAction SilentlyContinue
  Add-Check 'Obvious secret scan' (-not [bool]$secretHits) $(if ($secretHits) { ($secretHits.Path -join ', ') } else { 'no obvious secrets found' })

  $trackedGenerated = if ($git) { & git -c "safe.directory=$($Root -replace '\\','/')" ls-files output dl logs build 2>$null } else { @() }
  Add-Check 'Generated files not tracked' (-not [bool]$trackedGenerated) $(if ($trackedGenerated) { $trackedGenerated -join ', ' } else { 'none' })
} finally {
  Pop-Location
}

$header = @(
  'Linux-embarque modernization verification',
  "Generated: $([DateTime]::UtcNow.ToString('u'))",
  "Repository: $Root",
  ''
)
Set-Content -LiteralPath $Report -Value ($header + $Results + '', "Failures: $Failures") -Encoding utf8
$Results | ForEach-Object { Write-Host $_ }
Write-Host "Report: $Report"
if ($Failures -gt 0) { exit 1 }
