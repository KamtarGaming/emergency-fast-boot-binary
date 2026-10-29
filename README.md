# EMERGENCY Fast Boot Binary

Source code for the native component of **EMERGENCY Fast Boot**.

This project targets the Steam build of EMERGENCY identified by:

- Steam App ID: `850170`
- Steam build ID: `19075383`
- Shipping executable SHA-256: `2D59435BB10CB83A614EEB9B4FF241F857A0D67B0C91B423AF25BA51693F15B2`

It contains no game files, assets, proprietary SDK files, UE4SS files, AutoHotkey scripts, launcher, or background service.

## What the code does

`src/dwmapi_proxy.c` builds a local `dwmapi.dll` proxy. It forwards the four DWM functions imported by the shipping executable to the genuine Windows DLL. During startup it finds the EMERGENCY window, waits for Unreal Engine to complete its startup-window replacement, finds the current game window again, and queues a single Space key event within that game process. The startup photosensitivity screen accepts an ordinary key event.

`package.ps1` generates the mod layout and creates six zero-byte `.bk2` files; these replace the logo videos. It never reads or copies game assets.

## Requirements

- Windows x64
- PowerShell 5.1 or newer
- [Zig 0.17.0](https://ziglang.org/download/)

## Build

```powershell
git clone https://github.com/REPLACE_WITH_YOUR_ACCOUNT/emergency-fast-boot-binary.git
cd emergency-fast-boot-binary
.\build.ps1 -Zig 'C:\Tools\zig\zig.exe'
```

The compiled DLL is written to `build\dwmapi.dll`.

## Build the distributable layout

```powershell
.\package.ps1 -OutputDirectory .\dist
```

The resulting files are:

```text
dist\
  EMERGENCY\Binaries\Win64\dwmapi.dll
  EMERGENCY\Content\Movies\*.bk2
  manifest.json
```

Zip the **contents** of `dist`, then install by dragging the `EMERGENCY` folder into the game's Steam `common` folder and accepting file replacement.

## Smoke tests

The two small test programs check both explicit loading and ordinary import resolution of the proxy. Build them from the repository root after building the DLL:

```powershell
$zig = 'C:\Tools\zig\zig.exe'
New-Item -ItemType Directory -Force .\build\smoke | Out-Null
Copy-Item .\build\dwmapi.dll .\build\smoke\dwmapi.dll -Force
& $zig cc -target x86_64-windows-gnu -O2 .\test\proxy_smoke_test.c -o .\build\smoke\proxy_smoke_test.exe
& $zig cc -target x86_64-windows-gnu -O2 .\test\import_smoke_test.c -o .\build\smoke\import_smoke_test.exe -ldwmapi
Push-Location .\build\smoke
.\proxy_smoke_test.exe
.\import_smoke_test.exe
Pop-Location
```

Expected output contains `PROXY_SMOKE_TEST_OK` and `IMPORT_SMOKE_TEST_OK`.

## Updating the game

This mod is build-specific. Re-check the shipping executable hash and test a fresh launch after a game update, a Steam integrity check, or a reinstall.

## License

Source code in this repository is released under the MIT License. Game names and assets remain the property of their respective owners.
