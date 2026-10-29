param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$dll = Join-Path $root 'build\dwmapi.dll'
$expectedShippingExeHash = '2D59435BB10CB83A614EEB9B4FF241F857A0D67B0C91B423AF25BA51693F15B2'
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

$manifest = [ordered]@{
    name = 'EMERGENCY Fast Boot Binary'
    version = '1.0.2'
    steamAppId = 850170
    shippingExeSha256 = $expectedShippingExeHash
    dwmapiDllSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $bin 'dwmapi.dll')).Hash
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'manifest.json') -Encoding utf8
