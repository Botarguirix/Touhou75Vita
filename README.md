# Touhou75Vita

Porting research and runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita.

The current iteration integrates the pinned WinVita PE32 loader and Box86-derived ARMv7 dynamic recompiler. It maps the user's Japanese `TH075.exe` into guest memory, inventories its imports, and runs synthetic x86 routines through the dynarec and the real IAT slots for three diagnostic KERNEL32 shims. Iteration 07 passed the PE mapping and minimal dynarec smoke test on the user's physical Vita; Iteration 08 r1 failed its LastError test on hardware; r2 separates test code pages and unconditionally discards cached translations before writing synthetic code. The user's physical Vita log confirms r2 passes all three diagnostic calls, exact stack restoration, and the timer range check. Game startup and 154 remaining imports are still unsupported. It deliberately does not call the game's entry point or execute the English patch.

## Build the runtime-test VPK

Install VitaSDK, set `VITASDK`, build the pinned runtime, then build the Vita app:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is `vita/touhou75_vita.vpk`. GitHub Actions uses the same VitaSDK runtime build and publishes `Touhou75Vita-iteration08-r2-iat-bridge-vpk`.

## Try it on a Vita

1. Create `ux0:data/TH075Vita/` on the memory card.
2. Copy the Japanese game executable there as `TH075.exe`. Keep the optional English translation patch at `TH075E.exe`; it is not the game executable.
3. Install the VPK built from this revision with VitaShell and launch **Touhou 7.5 Vita - Iteration 08 r2 IAT Bridge Test**. In GitHub Actions, download the artifact named `Touhou75Vita-iteration08-r2-iat-bridge-vpk` from a successful run on the commit containing this code.
4. Open `ux0:data/TH075Vita/iteration08.log` in VitaShell. Confirm it contains `build_id=iteration08-winvita-iat-bridge-r2` and `result=pe_mapped_dynarec_and_iat_smoke_passed`, then share the full log.

The app returns to LiveArea after writing the report. That is expected; this iteration tests the loader and CPU engine, not game startup.

## Keep game data outside Git

The repository intentionally excludes proprietary files. Do not commit `TH075.exe`, `th075.dat`, `th075bgm.dat`, other game data, or generated large analysis streams. Provide files from your own game copy locally when a later runtime stage needs them.
