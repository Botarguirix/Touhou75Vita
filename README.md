# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical 86 confirmed original menu navigation and clean effects. The multicore raster introduced in 87 caused native startup regressions. Physical 89 now passes mutex/once/TLS, VM/JIT and original game initialization, but its first raw Vita raster thread crashes in pthread's cancellable wait because its OS pthread context was never registered. Iteration 90/version 01.94 removes the worker execution path and uses the serial renderer confirmed in 86, while retaining reduced diagnostic hashing and the corrected native TLS link. The ELF/SELF contains no RasterWorkers code. 110 portable groups, sanitizers, native build and package checks passed. Physical 90 recovery/performance/audio and playable combat remain pending. See [comparison, diagnosis and package](docs/STATUS_ITERATION90.md), [CPU/GPU plan](docs/PERFORMANCE_ITERATION87.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION90.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration90-serial-recovery-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 90 Serial Recovery**, package version 01.94, on the same Vita used previously. The screen must show ITERATION 90.
4. Allow up to 180 seconds for the bounded run (210-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration90.log, iteration90-runtime.log and iteration90-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration90-serial-recovery-r1, preflight/VM/JIT success and startup_raster_policy=serial_recovery with raster_workers:disabled. Record real Present frames, original title state, input edges/bindings, audio output timing and the final import/limit. Include log.txt and a screenshot. This run remains bounded to 360 Present / 361 waits / 180 seconds and may stop earlier at an unsupported service. A crash before watchdog creation may produce no watchdog log; preserve the new psp2dmp or .tmp if available. There is no iteration90-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend and sustained combat controls/audio remain future work. Complete opening music played without problems in physical 85; menu navigation and clean effects were confirmed in physical 86. Combat remains unverified. The menu boot preset intentionally omits the opening track. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.

For local imagegen artwork, encode the original background and icon masters with
`python tools/prepare_generated_livearea.py BACKGROUND_MASTER ICON_MASTER OUTPUT --iteration 89 --version 01.94`,
then configure `VITA_LIVEAREA_DIR` to that output directory. Use the masters on
every iteration; do not reuse already indexed package images. CMake checks the
iteration/version provenance and LiveArea content revision before packaging.
