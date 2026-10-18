$ErrorActionPreference = 'Stop'
$id = [Security.Principal.WindowsIdentity]::GetCurrent()
if (-not ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'Administrator rights are needed to write to Common Files. Relaunching elevated...'
    Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    exit
}
$src  = Join-Path $PSScriptRoot 'Lumivore.vst3'
$dest = Join-Path $env:CommonProgramFiles 'VST3'
if (-not (Test-Path $src)) { throw "Lumivore.vst3 not found next to this script." }
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item $src $dest -Recurse -Force
Write-Host "Installed to $dest\Lumivore.vst3"
Write-Host 'Now rescan plugins in FL Studio (see README.txt).'
Read-Host 'Press Enter to close'
