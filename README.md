# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 85 reached 339 real Present frames and 2999 draws. Its original TitleScene ran across 97 observed waits; the user saw part of the menu and heard complete music without problems. All 343 main-wait and 626 audio-dispatch contexts were preserved. Raster/hashing still took 99.620 of 180.532 s; final graphics usage dropped to 28.1 MiB from a 40.9 MiB peak. The run stopped at the time cap. At the user's request, iteration 86/version 01.90 applies four fingerprint-checked changes (ten modified bytes) to the mapped Japanese EXE before execution: shorten the logo, route its original transition to title, and keep menu input available without timed attract demos. The disk EXE/DAT remain unchanged. Opening and its music are omitted; original menu initialization, drawing, input and cleanup still execute. Ninety-six portable groups, local original fingerprints, sanitizers, oracle/replay comparisons, VitaSDK compilation and package validations passed. Controls already map Vita buttons to keyboard state and now have shipping COM tests and key-edge logs. Hardware 86/menu interaction and combat remain pending. This removes intro work; it does not accelerate the CPU renderer per frame. See [scope, controls and package](docs/STATUS_ITERATION86.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION86.md), [research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration86-menu-boot-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 86 Menu Boot**, package version 01.90, on the same Vita used previously. The screen must show ITERATION 86.
4. Allow up to 180 seconds for the bounded run (210-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration86.log, iteration86-runtime.log and iteration86-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration86-menu-boot-r1 and include the game's log.txt and a screenshot. Inspect startup_intro_patch, startup_frame_title_state, startup_dinput_key, startup_dinput_bindings, startup_frame_wait_stop, startup_d3d8_present_scope_limit, startup_dsound_pcm_lock offset/flags/pointers, startup_d3d8_draw_geometry, startup_d3d8_vertex_bits, startup_dsound_output_timing, startup_audio_running_lateness_us, startup_audio_trap_yield/context/dispatches, startup_d3d8_linear_work, startup_d3d8_draw_filter, startup_d3d8_draw_hash, startup_dsound_play, startup_dsound_output_pipeline/summary, startup_dsound_pcm_cursor, startup_audio_worker_dispatches, worker_resumed_com, startup_dsound_pcm_format (format_bytes_read:16 / cbsize:ignored_pcm), startup_dsound_secondary, startup_dsound_pcm_qi/lock/upload/contract, startup_native_heap, startup_kernel_memory, startup_d3d8_storage_summary/peak, startup_d3d8_allocation_denied, startup_messagebox_text/caption, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase and preserved context; then present=executed and the final startup_stop_import/EIP. It can present up to 360 real frames with 361 original wait resumes, then stops before the 361st Present; the time cap or an unsupported service can stop it sooner. There is no iteration86-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend and sustained combat controls/audio remain future work. Complete opening music played without problems in physical 85; combat audio and interactive selections remain unverified. The menu boot preset intentionally omits the opening track. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
