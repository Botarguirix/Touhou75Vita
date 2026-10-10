# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 83 reached 228 real Present frames and 695 draws, showing a new opening scene. Its first 208 scanout hashes and first 397 draw hashes match 82; those same draws took 43.172 s instead of 68.308 s. All 230 main-wait and 189 audio-dispatch contexts were preserved. Original music lasted longer but remained choppy, and the user reported maximum CPU usage. Execution stopped before a slightly skewed DrawPrimitiveUP strip (request 696). Iteration 84/version 01.88 implements strictly convex triangle strips with affine UVs and top-left coverage, retaining the rectangular path, and measures native audio submission gaps, blocking and silence. It keeps 240 real frames and allows 130 s (150-second watchdog) to exercise the new boundary. LiveArea uses Alice freshly decoded from the user's original DAT. Seventy-seven portable groups, 500 deterministic strips against an independent oracle, 200000 bilinear sample comparisons, a 1312-call ownership replay, POINT/LINEAR/triangle -O0/-O2 comparisons, VitaSDK compilation and package checks passed. Hardware 84, continuous audio, menu interaction and playability remain pending. See [scope/package](docs/STATUS_ITERATION84.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION84.md), [research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration84-triangle-strips-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 84 Triangle Strips**, package version 01.88, on the same Vita used previously. The screen must show ITERATION 84.
4. Allow up to 130 seconds for the bounded run (150-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration84.log, iteration84-runtime.log and iteration84-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration84-triangle-strips-r1 and include the game's log.txt and a screenshot. Inspect startup_d3d8_draw_geometry, startup_d3d8_vertex_bits, startup_dsound_output_timing, startup_audio_running_lateness_us, startup_audio_trap_yield/context/dispatches, startup_d3d8_linear_work, startup_d3d8_draw_filter, startup_d3d8_draw_hash, startup_dsound_play, startup_dsound_output_pipeline/summary, startup_dsound_pcm_cursor, startup_audio_worker_dispatches, worker_resumed_com, startup_dsound_pcm_format (format_bytes_read:16 / cbsize:ignored_pcm), startup_dsound_secondary, startup_dsound_pcm_qi/lock/upload/contract, startup_native_heap, startup_kernel_memory, startup_d3d8_storage_summary/peak, startup_d3d8_allocation_denied, startup_messagebox_text/caption, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase and preserved context; then present=executed and the final startup_stop_import/EIP. It can present up to 240 real frames, then stops before the 241st Present; the time cap or an unsupported service can stop it sooner. There is no iteration84-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and sustained guest audio remain future work. Native guest PCM lasted longer but remained choppy in physical 83; sustained playback and interactive scenes remain unverified. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
