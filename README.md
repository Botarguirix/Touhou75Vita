# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 79 allocated and uploaded all 24 opening.dat textures, matching the projected 42896712 bytes of live graphics storage. All 203 scanout hashes, timer signals and preserved main continuations matched 78. Native diagnostics confirmed a 128 MiB reserved heap with 57415616 bytes in use at the stop. The next original request is a 1 MiB stereo PCM buffer at 44100 Hz / 16 bits. The bridge incorrectly treated the following RIFF data-chunk bytes as a required zero cbSize. Iteration 80/version 01.84 reads only the 16-byte PCM prefix; Microsoft documents cbSize as ignored for PCM. Format, alignment, capacity and ownership checks remain enforced. Forty-one focused portable groups, the 1312-call ownership replay and the -O0/-O2 renderer comparison passed; VitaSDK compilation/package inspection passed. Hardware 80, original BGM playback and opening scene rendering are pending. The run remains bounded to 240 real frames/waits, 100 s and a 120 s watchdog. See [scope](docs/STATUS_ITERATION80.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION80.md), [research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration80-pcm-format-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 80 PCM Format**, package version 01.84, on the same Vita used previously. The screen must show ITERATION 80.
4. Allow up to 100 seconds for the bounded run (120-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration80.log, iteration80-runtime.log and iteration80-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration80-pcm-format-r1 and include the game's log.txt and a screenshot. Inspect startup_dsound_pcm_format (format_bytes_read:16 / cbsize:ignored_pcm), startup_dsound_secondary, startup_dsound_pcm_qi/lock/upload/contract, startup_native_heap, startup_kernel_memory, startup_d3d8_storage_summary/peak, startup_d3d8_allocation_denied, startup_messagebox_text/caption, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase and preserved context; then present=executed and the final startup_stop_import/EIP. It can present up to 240 real frames, then stops before the 241st Present; the time cap or an unsupported service can stop it sooner. There is no iteration80-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
