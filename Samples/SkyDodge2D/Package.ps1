param(
    [string]$ExecutablePath
)

$ErrorActionPreference = 'Stop'
$sampleRoot = $PSScriptRoot
$engineRoot = (Resolve-Path (Join-Path $sampleRoot '../..')).Path
if (-not $ExecutablePath) {
    $ExecutablePath = Join-Path $engineRoot 'build-skydodge-portable/bin/Release/SkyDodge2D.exe'
}
if (-not (Test-Path -LiteralPath $ExecutablePath -PathType Leaf)) {
    throw "Build SkyDodge2D Release first: $ExecutablePath"
}

$packageRoot = Join-Path $sampleRoot 'Builds/Windows-x64-Release'
New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot 'Scenes') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot 'Assets') | Out-Null
Copy-Item -LiteralPath $ExecutablePath -Destination (Join-Path $packageRoot 'SkyDodge2D.exe') -Force
Copy-Item -LiteralPath (Join-Path $sampleRoot 'jevaing.project') -Destination $packageRoot -Force
Copy-Item -LiteralPath (Join-Path $sampleRoot 'Scenes/main.scene') -Destination (Join-Path $packageRoot 'Scenes') -Force

$archive = Join-Path $sampleRoot 'Builds/SkyDodge2D-Windows-x64-Release.zip'
Compress-Archive -Path (Join-Path $packageRoot '*') -DestinationPath $archive -Force
Write-Output $archive
