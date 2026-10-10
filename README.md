# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 76 confirmed sixteen original EXE Present calls, sixteen signals/waits from the original timer worker and preserved main contexts. The first eight scanout hashes matched iteration 75, and the initial fade completed at frame 11. It stopped at the deliberate seventeenth Present cap in 21.21 s, with no CPU fault/limit and a disarmed watchdog. Static analysis shows the logo must then run 181 original updates before requesting scene 0x0D. Iteration 77/version 01.81 permits up to 240 real frames/waits within 75 s (90 s watchdog), caches native scanout conversion only for byte-identical confirmed backbuffers and observes the original scene/age. Twenty-three portable check groups and the -O0/-O2 renderer digest comparison passed on PC. Physical 77 and the scene handoff are pending. See [scope](docs/STATUS_ITERATION77.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION77.md), [decompilation research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration77-exe-logo-handoff-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 77 EXE Logo Handoff**, package version 01.81, on the same Vita used previously. The screen must show ITERATION 77.
4. Allow up to 75 seconds for the bounded run (90-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration77.log, iteration77-runtime.log and iteration77-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration77-exe-logo-handoff-r1 and include the game's log.txt and a screenshot. Inspect startup_d3d8_present_conversion, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase, timings, preserved context and consumed frame waits; then present=executed and the final startup_stop_import/EIP. It can present up to 240 real frames, then stops before the 241st Present; the time cap or an unsupported service can stop it sooner. There is no iteration77-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
