# Touhou75Vita

Porting research and runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita.

The current iteration integrates the pinned WinVita PE32 loader and Box86-derived ARMv7 dynamic recompiler. It maps the user's Japanese `TH075.exe` into guest memory, inventories its imports, and runs a tiny synthetic x86 smoke routine through the dynarec. It deliberately does not call the game's entry point or execute the English patch.

## Build the runtime-test VPK

Install VitaSDK, set `VITASDK`, build the pinned runtime, then build the Vita app:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is `vita/touhou75_vita.vpk`. GitHub Actions uses the same VitaSDK runtime build and publishes `Touhou75Vita-iteration07-armv7-dynarec-smoke-vpk`.

## Try it on a Vita

1. Create `ux0:data/TH075Vita/` on the memory card.
2. Copy the Japanese game executable there as `TH075.exe`. Keep the optional English translation patch at `TH075E.exe`; it is not the game executable.
3. Install the VPK built from this revision with VitaShell and launch **Touhou 7.5 Vita - Iteration 07 ARMv7 Runtime Test**. In GitHub Actions, download the artifact named `Touhou75Vita-iteration07-armv7-dynarec-smoke-vpk` from a successful run on the commit containing this code.
4. Open `ux0:data/TH075Vita/iteration07.log` in VitaShell. Confirm it contains `build_id=iteration07-winvita-armv7-smoke-r1` and `dynarec_smoke_result=passed`, then share the full log.

The app returns to LiveArea after writing the report. That is expected; this iteration tests the loader and CPU engine, not game startup.

## Keep game data outside Git

The repository intentionally excludes proprietary files. Do not commit `TH075.exe`, `th075.dat`, `th075bgm.dat`, other game data, or generated large analysis streams. Provide files from your own game copy locally when a later runtime stage needs them.
