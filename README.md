# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 10 passed identity, CPU, IAT, heap, file, TEB/FS, dynamic TLS, basic process and visible-screen checks on the user's physical Vita. Iteration 11 is prepared locally to invoke the original Japanese EXE's entry point and stop before servicing its first import. Its build and hardware validation are pending. This checkpoint does not establish that the full game boots. TLS callbacks and the English patch remain unexecuted. See [Iteration 11 scope and expected results](docs/STATUS_ITERATION11.md) and [Iteration 10 hardware evidence](docs/STATUS_ITERATION10.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk. Actions builds pushes to main and codex/d2vita-runtime-review and publishes **Touhou75Vita-iteration11-exe-startup-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 11 EXE Startup**, package version 01.13, on the same Vita used previously.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Share ux0:data/TH075Vita/iteration11.log, iteration11-runtime.log and iteration11-watchdog.log. Confirm build_id=iteration11-original-entrypoint-first-import-r1. Expected result: real_entrypoint_first_import_passed, with startup_first_import=KERNEL32.dll!GetVersionExA. Display reports screen_result=presented separately. If the watchdog closes the app, share the available logs instead.

The screen shows preflight and original-entrypoint checkpoint results. The first import is intercepted without a fabricated API return. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 11 needs only the known Japanese EXE and verifies its hash before PE loading.
