param(
    [Parameter(Mandatory = $true)]
    [string]$Zig
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $out | Out-Null

$previousGlobalCache = $env:ZIG_GLOBAL_CACHE_DIR
$previousLocalCache = $env:ZIG_LOCAL_CACHE_DIR
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $out 'zig-global-cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $out 'zig-local-cache'

& $Zig cc -target x86_64-windows-gnu -O2 -static -shared `
    (Join-Path $root 'src\dwmapi_proxy.c') `
    -o (Join-Path $out 'dwmapi.dll') `
    -luser32 -lkernel32

$env:ZIG_GLOBAL_CACHE_DIR = $previousGlobalCache
$env:ZIG_LOCAL_CACHE_DIR = $previousLocalCache

if ($LASTEXITCODE -ne 0) {
    throw "Zig compilation failed with exit code $LASTEXITCODE."
}
