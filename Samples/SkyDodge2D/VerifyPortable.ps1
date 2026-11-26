param(
    [string]$ExecutablePath
)

$ErrorActionPreference = 'Stop'
$sampleRoot = $PSScriptRoot
$engineRoot = (Resolve-Path (Join-Path $sampleRoot '../..')).Path
if (-not $ExecutablePath) {
    $ExecutablePath = Join-Path $engineRoot 'build-skydodge-portable/bin/Release/SkyDodge2D.exe'
}
$ExecutablePath = (Resolve-Path -LiteralPath $ExecutablePath).Path

$dumpbinCommand = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if ($dumpbinCommand) {
    $dumpbin = $dumpbinCommand.Source
} else {
    $visualStudioRoot = Join-Path $env:ProgramFiles 'Microsoft Visual Studio'
    $dumpbin = Get-ChildItem -LiteralPath $visualStudioRoot -Filter dumpbin.exe -Recurse |
        Where-Object { $_.FullName -match 'Hostx64\\x64\\dumpbin\.exe$' } |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $dumpbin) { throw 'dumpbin.exe is required to verify portable imports.' }

$dependencies = & $dumpbin /DEPENDENTS $ExecutablePath | Out-String
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect executable dependencies.' }
$imports = [regex]::Matches($dependencies, '(?im)^\s*([A-Za-z0-9_.-]+\.dll)\s*$') |
    ForEach-Object { $_.Groups[1].Value }
$windowsLibraries = @('USER32.dll', 'GDI32.dll', 'd3d11.dll',
    'D3DCOMPILER_47.dll', 'XINPUT1_4.dll', 'KERNEL32.dll')
if ($imports.Count -eq 0) { throw 'No PE imports were found; check dumpbin output.' }
foreach ($import in $imports) {
    if ($windowsLibraries -notcontains $import) {
        throw "Unexpected runtime DLL: $import"
    }
}
Write-Output 'PASS: only Windows system DLL imports.'

& $ExecutablePath --logic-test
if ($LASTEXITCODE -ne 0) { throw 'Game logic test failed.' }
& $ExecutablePath --renderer null --frames 2
if ($LASTEXITCODE -ne 0) { throw 'Game runtime test failed.' }

$archive = & (Join-Path $sampleRoot 'Package.ps1') -ExecutablePath $ExecutablePath |
    Select-Object -Last 1
$extract = Join-Path $env:TEMP ('jevaing-skydodge-portable-' + [guid]::NewGuid().ToString('N'))
try {
    Expand-Archive -LiteralPath $archive -DestinationPath $extract
    & (Join-Path $extract 'SkyDodge2D.exe') --renderer null --frames 2
    if ($LASTEXITCODE -ne 0) { throw 'Extracted portable package failed.' }
    Write-Output 'PASS: extracted ZIP runs outside the source tree.'
} finally {
    $tempRoot = [System.IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
    $extractRoot = [System.IO.Path]::GetFullPath($extract)
    if ($extractRoot.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $extractRoot)) {
        Remove-Item -LiteralPath $extractRoot -Recurse -Force
    }
}
