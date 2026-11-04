param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$dll = Join-Path $root 'build\dwmapi.dll'
$movieNames = @(
    'BMWELogoDE.bk2',
    'BMWELogoEN.bk2',
    'EMERGENCYLogo.bk2',
    'NVidiaLogo.bk2',
    'SixteenTonsEntertainmentLogo.bk2',
    'UnrealLogo.bk2'
)

if (-not (Test-Path -LiteralPath $dll)) {
    throw 'build\\dwmapi.dll is missing. Run build.ps1 first.'
}
if (Test-Path -LiteralPath $OutputDirectory) {
    throw "Output directory already exists: $OutputDirectory"
}

$bin = Join-Path $OutputDirectory 'EMERGENCY\Binaries\Win64'
$movies = Join-Path $OutputDirectory 'EMERGENCY\Content\Movies'
New-Item -ItemType Directory -Force -Path $bin,$movies | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $bin 'dwmapi.dll')
foreach ($movie in $movieNames) {
    New-Item -ItemType File -Path (Join-Path $movies $movie) | Out-Null
}
