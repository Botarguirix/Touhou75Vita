# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Iteration 12 is prepared locally and awaits an Actions build and hardware validation. It serves the observed GetVersionExA and GetModuleHandleA(NULL) contracts, resumes original EXE instructions, and stops before HeapCreate. It checks the guest's consumption of the returned version data, stdcall returns and the next call frame. Full game boot remains unverified. See [Iteration 12 scope and procedure](docs/STATUS_ITERATION12.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions builds pushes to main and codex/d2vita-runtime-review and publishes **Touhou75Vita-iteration12-startup-services-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 12 Startup Services**, package version 01.15, on the same Vita used previously. The screen must show ITERATION 12.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Share ux0:data/TH075Vita/iteration12.log, iteration12-runtime.log and iteration12-watchdog.log. Confirm build_id=iteration12-startup-version-module-r1. Expected result: real_entrypoint_version_module_passed, with startup_serviced_imports=2 and startup_stop_import=KERNEL32.dll!HeapCreate. Display reports screen_result=presented separately. If the watchdog closes the app, share the available logs instead.

The screen shows preflight and original-startup checkpoint results. Two narrowly checked API variants are serviced; other imports stop execution with a diagnostic. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 12 needs only the known Japanese EXE and verifies its hash before PE loading.
