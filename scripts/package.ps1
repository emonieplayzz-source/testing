$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vst  = Join-Path $root 'build/Lumivore_artefacts/Release/VST3/Lumivore.vst3'
$dll  = Join-Path $vst  'Contents/x86_64-win/Lumivore.vst3'
if (-not (Test-Path $dll)) { throw "VST3 binary not found: $dll (build first)" }
$rel = Join-Path $root 'release'
if (Test-Path $rel) { Remove-Item $rel -Recurse -Force }
New-Item -ItemType Directory $rel | Out-Null
Copy-Item $vst $rel -Recurse
Copy-Item (Join-Path $root 'README.txt') $rel
Copy-Item (Join-Path $root 'docs/TROUBLESHOOTING_FL_STUDIO.txt') $rel
Copy-Item (Join-Path $PSScriptRoot 'install.ps1') $rel
Copy-Item (Join-Path $PSScriptRoot 'install.bat') $rel
$zip = Join-Path $root 'Lumivore-Windows-x64.zip'
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $rel '*') -DestinationPath $zip
Write-Host "Packaged: $zip"
