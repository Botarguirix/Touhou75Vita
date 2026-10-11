# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical 86 confirmed original menu navigation and clean effects. Physical 87 omitted the pthread_cancel activation marker and crashed in its first worker's mutex wait. Physical 88 passed its mutex check but crashed earlier in the JIT's libgcc emutls path: pthread_once was also omitted, leaving its TLS mutex uninitialized. Iteration 89/version 01.93 includes those functions and pthread_cond_destroy, checks once/TLS before the VM and in each raster worker, and preserves flushed startup checkpoints. 109 portable groups, sanitizers, native ELF inspection and package checks passed. Fresh Reimu/Marisa LiveArea artwork and a Reimu icon are encoded from original imagegen masters; artwork metadata must match the package version. Physical 89 boot/performance/audio and playable combat remain pending. See [diagnosis and package](docs/STATUS_ITERATION89.md), [CPU/GPU plan](docs/PERFORMANCE_ITERATION87.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION89.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration89-tls-startup-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 89 TLS Startup**, package version 01.93, on the same Vita used previously. The screen must show ITERATION 89.
4. Allow up to 180 seconds for the bounded run (210-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration89.log, iteration89-runtime.log and iteration89-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration89-tls-startup-r1 and include the game's log.txt and a screenshot. First inspect startup_native_thread_once, startup_native_thread_tls, startup_native_thread_preflight, startup_runtime_init and startup_dynarec_smoke. Then inspect startup_raster_worker_create/start, startup_raster_worker (verified:yes / tls:passed on cores 1/2), startup_raster_parallel_summary, startup_runner_cpu_id, startup_intro_patch, startup_frame_title_state, startup_dinput_key, startup_dinput_bindings, startup_frame_wait_stop, startup_d3d8_present_scope_limit, startup_dsound_pcm_lock offset/flags/pointers, startup_d3d8_draw_geometry, startup_d3d8_vertex_bits, startup_dsound_output_timing, startup_audio_running_lateness_us, startup_audio_trap_yield/context/dispatches, startup_d3d8_linear_work, startup_d3d8_draw_filter, startup_d3d8_draw_hash, startup_dsound_play, startup_dsound_output_pipeline/summary, startup_dsound_pcm_cursor, startup_audio_worker_dispatches, worker_resumed_com, startup_dsound_pcm_format (format_bytes_read:16 / cbsize:ignored_pcm), startup_dsound_secondary, startup_dsound_pcm_qi/lock/upload/contract, startup_native_heap, startup_kernel_memory, startup_d3d8_storage_summary/peak, startup_d3d8_allocation_denied, startup_messagebox_text/caption, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase and preserved context; then present=executed and the final startup_stop_import/EIP. It can present up to 360 real frames with 361 original wait resumes, then stops before the 361st Present; the time cap or an unsupported service can stop it sooner. There is no iteration89-bgm.log. A crash before worker startup may produce no watchdog log; preserve the new psp2dmp, including a .tmp file if that is all the Vita saved.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend and sustained combat controls/audio remain future work. Complete opening music played without problems in physical 85; menu navigation and clean effects were confirmed in physical 86. Combat remains unverified. The menu boot preset intentionally omits the opening track. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.

For local imagegen artwork, encode the original background and icon masters with
`python tools/prepare_generated_livearea.py BACKGROUND_MASTER ICON_MASTER OUTPUT --iteration 89 --version 01.93`,
then configure `VITA_LIVEAREA_DIR` to that output directory. Use the masters on
every iteration; do not reuse already indexed package images. CMake checks the
iteration/version provenance and LiveArea content revision before packaging.
