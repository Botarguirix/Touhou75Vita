# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical 95 presents character selection in Practice and Story, then stops at KERNEL32!GetTimeZoneInformation. Iteration96/version 02.00 implements that observed service from a Vita RTC UTC/local snapshot, with explicit 172-byte PE32 serialization, fixed-current-offset policy without annual DST rules, readback, LastError behavior and stdcall verification. 168 portable C++ groups and 11 Python checks, sanitizers, native build and package checks passed. Only two official SceRtcUser functions were added; other imports and SELF memory controls are preserved. Physical continuation into combat and 60 FPS remain pending. See [iteration96 report](docs/STATUS_ITERATION96.md), [roadmap](docs/ROADMAP_AFTER_SELECTION95.md), [physical95 result](docs/STATUS_ITERATION95.md) and [60 FPS measurements](docs/PERFORMANCE_60FPS.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration96-time-zone-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 96 Time Zone**, package version 02.00, on the same Vita used previously. The screen must show ITERATION 96.
4. Select Practice Mode and confirm characters/stage with Cross/Z. If combat appears, check movement and one attack. Save its logs separately before repeating Story. Record black/diagnostic behavior and the selected option. This test permits 1800 Present or 300 seconds (330-second watchdog). Photograph the capture/counter, then press Cross on the diagnostic to exit.
5. Inspect iteration96.log, iteration96-runtime.log and iteration96-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration96-time-zone-r1, preflight/VM/JIT success and startup_raster_policy=serial_half_resolution with raster_workers:disabled. Confirm pixel_probes:disabled and collect startup_d3d8_alpha_summary, startup_60fps_all_cycles, startup_dinput_summary and startup_session_scene_summary. Record real Present frames, original title state, input edges/bindings, audio output timing and the final import/limit. Include log.txt and a screenshot. This run remains bounded to 1800 Present / 1801 waits / 300 seconds and may stop earlier at an unsupported service. A crash before watchdog creation may produce no watchdog log; preserve the new psp2dmp or .tmp if available. There is no iteration96-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend and sustained combat controls/audio remain future work. Complete opening music played without problems in physical 85; menu navigation and clean effects were confirmed in physical 86. Combat remains unverified. The menu boot preset intentionally omits the opening track. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.

For local original DAT artwork, run
`python tools/prepare_dat_livearea.py USER_TH075_DAT OUTPUT --frame 6 --iteration 94 --version 02.00 --font FONT_PATH`,
then configure `VITA_LIVEAREA_DIR` to that output directory. Each build decodes
the original character frame again; do not reuse indexed package PNGs. Icon
and startup artwork center the visible alpha bounds. CMake checks iteration,
version provenance and LiveArea content revision before packaging.
