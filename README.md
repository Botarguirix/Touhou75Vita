# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 74 executed PeekMessageA, returned from the empty queue and presented one original EXE frame. Vita display set/vblank/query returned zero and the active framebuffer matched. Main then blocked on its unsignaled frame event at WaitForSingleObject, return 603545. Iteration 75/version 01.79 resumes the original timer worker after its real 16 ms timeout, requires its SetEvent signal and consumes that event before continuing main. It checks main GPR/flags/TIB/FPU, call frame and LastError across the handoff. Eighteen focused portable check groups passed on PC; the new worker/main integration needs physical Vita validation. See [scope](docs/STATUS_ITERATION75.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION75.md), [decompilation research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration75-exe-frame-events-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 75 EXE Frame Events**, package version 01.79, on the same Vita used previously. The screen must show ITERATION 75.
4. Wait for the results screen, photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration75.log, iteration75-runtime.log and iteration75-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration75-exe-frame-events-r1 and include the game's log.txt and a screenshot. Inspect startup_frame_wait_context=preserved, startup_event_signal from timer TIB 760000, startup_frame_wait_resume=passed and startup_frame_wait_resumes; then present=executed and the final startup_stop_import/EIP. It can present up to eight real frames, then stops before the ninth Present; unsupported earlier services may stop it sooner. There is no iteration75-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
