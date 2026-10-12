# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical 93 recorded Cross/Z, then rejected the last 2 MiB character-selection texture at the bridge's 64 MiB graphics cap; the original EXE reported a texture-creation error. Iteration 94/version 01.98 raises the bounded cap to 96 MiB within the existing 128 MiB native heap. It enters the original title constructor directly, skips the logo/opening and starts title layers opaque. Local LiveArea/icon/startup art comes freshly from Alice's original DAT portrait, with centered icons. 151 portable C++ groups and 11 Python checks, sanitizers, native build and package checks passed. Physical selection, playable combat and 60 FPS remain pending. See [iteration report](docs/STATUS_ITERATION94.md), [playable path and resources](docs/PLAYABLE_PATH.md), [physical 93 report](docs/STATUS_ITERATION93.md), [roadmap](docs/ROADMAP.md) and [60 FPS target](docs/PERFORMANCE_60FPS.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration94-selection-memory-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 94 Selection Memory**, package version 01.98, on the same Vita used previously. The screen must show ITERATION 94.
4. Select Practice Mode and confirm with Cross/Z; if selection appears, choose a character. Record black/diagnostic behavior and the selected option. This test permits 1800 Present or 300 seconds (330-second watchdog). Photograph the capture/counter, then press Cross on the diagnostic to exit.
5. Inspect iteration94.log, iteration94-runtime.log and iteration94-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration94-selection-memory-r1, preflight/VM/JIT success and startup_raster_policy=serial_half_resolution with raster_workers:disabled. Confirm pixel_probes:disabled and collect startup_d3d8_alpha_summary, startup_60fps_all_cycles, startup_dinput_summary and startup_session_scene_summary. Record real Present frames, original title state, input edges/bindings, audio output timing and the final import/limit. Include log.txt and a screenshot. This run remains bounded to 1800 Present / 1801 waits / 300 seconds and may stop earlier at an unsupported service. A crash before watchdog creation may produce no watchdog log; preserve the new psp2dmp or .tmp if available. There is no iteration94-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend and sustained combat controls/audio remain future work. Complete opening music played without problems in physical 85; menu navigation and clean effects were confirmed in physical 86. Combat remains unverified. The menu boot preset intentionally omits the opening track. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.

For local original DAT artwork, run
`python tools/prepare_dat_livearea.py USER_TH075_DAT OUTPUT --frame 6 --iteration 94 --version 01.98 --font FONT_PATH`,
then configure `VITA_LIVEAREA_DIR` to that output directory. Each build decodes
the original character frame again; do not reuse indexed package PNGs. Icon
and startup artwork center the visible alpha bounds. CMake checks iteration,
version provenance and LiveArea content revision before packaging.
