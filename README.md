# Touhou75Vita

Porting research and runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita.

The current Vita build is a diagnostic VPK. It checks that the homebrew starts, writes a log to the memory card, and recognizes the PE32/x86 headers of a user-supplied `TH075.exe`. It does not execute the game yet.

## Build the diagnostic VPK

Install VitaSDK, set `VITASDK`, and build from this directory:

```sh
cd vita
make
```

The package is `vita/touhou75_vita.vpk`. The same build runs in the VitaSDK container used by GitHub Actions.

## Try it on a Vita

1. Create `ux0:data/TH075Vita/` on the memory card.
2. Copy the Japanese game executable there as `TH075.exe`. Keep the optional English translation patch at `TH075E.exe`; it is not the game executable.
3. Install the VPK built from this revision with VitaShell and launch **Touhou 7.5 Vita - Iteration 06 Diagnostic**. In GitHub Actions, download the artifact named `Touhou75Vita-iteration06-vpk` from a successful run on the commit containing this code.
4. Open `ux0:data/TH075Vita/iteration06.log` in VitaShell. Confirm it contains `build_id=iteration06-dual-pe-probe-r1`, then share the full log.

The app returns to LiveArea after writing the report. That is expected in this diagnostic build.

## Keep game data outside Git

The repository intentionally excludes proprietary files. Do not commit `TH075.exe`, `th075.dat`, `th075bgm.dat`, other game data, or generated large analysis streams. Provide files from your own game copy locally when a later runtime stage needs them.
