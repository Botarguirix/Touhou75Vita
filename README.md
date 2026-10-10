# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 84 reached 239 real Present frames and 757 draws, including six convex strips and another opening image. All 240 main-wait and 221 cooperative audio contexts were preserved; graphics used 42896712 bytes of 64 MiB. It stopped at the 240-resume scope, whose initial wait produces no Present. Music progressed but was choppy with reported CPU load. Native submissions were regular, yet 1048 of 2170 blocks were silent. The bridge incorrectly ignored nonzero offsets in 53 ENTIREBUFFER locks. Iteration 85/version 01.89 preserves those offsets and wrapped spans, and extends the bounded original run to 360 Present / 361 resumes / 180 s (210-second watchdog). Raster/hashing still accounted for 63.279 of 104.133 s. Eighty portable groups, an old-code regression, sanitizer checks, oracle/replay comparisons, VitaSDK compilation and package checks passed. LiveArea uses Alice freshly decoded from the original DAT. Hardware 85, uninterrupted audio, interactive menu and playability remain pending. See [scope/package](docs/STATUS_ITERATION85.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION85.md), [research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration85-stream-offsets-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 85 Stream Offsets**, package version 01.89, on the same Vita used previously. The screen must show ITERATION 85.
4. Allow up to 180 seconds for the bounded run (210-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration85.log, iteration85-runtime.log and iteration85-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration85-stream-offsets-r1 and include the game's log.txt and a screenshot. Inspect startup_frame_wait_stop, startup_d3d8_present_scope_limit, startup_dsound_pcm_lock offset/flags/pointers, startup_d3d8_draw_geometry, startup_d3d8_vertex_bits, startup_dsound_output_timing, startup_audio_running_lateness_us, startup_audio_trap_yield/context/dispatches, startup_d3d8_linear_work, startup_d3d8_draw_filter, startup_d3d8_draw_hash, startup_dsound_play, startup_dsound_output_pipeline/summary, startup_dsound_pcm_cursor, startup_audio_worker_dispatches, worker_resumed_com, startup_dsound_pcm_format (format_bytes_read:16 / cbsize:ignored_pcm), startup_dsound_secondary, startup_dsound_pcm_qi/lock/upload/contract, startup_native_heap, startup_kernel_memory, startup_d3d8_storage_summary/peak, startup_d3d8_allocation_denied, startup_messagebox_text/caption, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase and preserved context; then present=executed and the final startup_stop_import/EIP. It can present up to 360 real frames with 361 original wait resumes, then stops before the 361st Present; the time cap or an unsupported service can stop it sooner. There is no iteration85-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and sustained guest audio remain future work. Native guest PCM progressed but remained choppy in physical 84; sustained playback and interactive scenes remain unverified. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
