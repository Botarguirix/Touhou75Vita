# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Hardware iteration 29 passed the module alias and processor query, recorded 70 counted startup service calls, and stopped at SetUnhandledExceptionFilter. Iteration 30 stores and returns the previous guest exception-filter pointer; exception dispatch remains unimplemented. Full game boot remains unverified. See [Iteration 30 scope](docs/STATUS_ITERATION30.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration30-r2-exception-filter-startup-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 30 Exception Filter**, package version 01.34, on the same Vita used previously. The screen must show ITERATION 30 R2.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Inspect iteration30.log, iteration30-runtime.log and iteration30-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration30-exception-filter-startup-r2 and startup_serviced_import=KERNEL32.dll!SetUnhandledExceptionFilter. The goal is filter registration and a later startup boundary; full boot remains unverified.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 30 needs only the known Japanese EXE and verifies its hash before PE loading.
