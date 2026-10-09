# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 75 presented eight original EXE frames, serviced eight signals from the original timer worker and consumed eight frame-event waits with main context preserved. It stopped at the deliberate ninth Present cap, with no CPU fault/limit and a disarmed watchdog. Iteration 76/version 01.80 enables an optimized native build (-O2 with strict floating-point options), permits up to sixteen real frames/waits and records draw/present/cycle times plus the original transition counter. Eighteen portable check groups passed on PC; renderer output digests matched between -O0 and -O2. The extended transition and performance still need physical validation. See [scope](docs/STATUS_ITERATION76.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION76.md), [decompilation research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration76-exe-startup-transition-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 76 EXE Transition**, package version 01.80, on the same Vita used previously. The screen must show ITERATION 76.
4. Wait for the results screen, photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration76.log, iteration76-runtime.log and iteration76-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration76-exe-startup-transition-r1 and include the game's log.txt and a screenshot. Inspect startup_native_optimization=enabled, draw/present/cycle elapsed_us, startup_frame_phase, preserved context and consumed frame waits; then present=executed and the final startup_stop_import/EIP. It can present up to sixteen real frames, then stops before the seventeenth Present; unsupported earlier services may stop it sooner. There is no iteration76-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
