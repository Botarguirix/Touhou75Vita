# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 77 confirmed 203 original EXE Present calls and 203 real timer/main continuations, with preserved main contexts and successful native scanout confirmation. The original logo reached age 181 and faded out completely. Loading the next scene stopped at MessageBoxA after a 1 MiB CreateTexture hit the 32 MiB bridge budget; the released logo still retained 2 MiB through an incorrect cached-surface reference. Iteration 78/version 01.82 forwards level-surface references to their texture and destroys storage at the last owned reference while preserving bindings/targets/state blocks. Thirty-four focused portable groups, a 1,286-call ownership replay from the physical log and the -O0/-O2 renderer comparison passed on PC. The replay now frees the logo and services the previously rejected allocation. The bounded run permits 240 real frames/waits within 100 s (120 s watchdog); MessageBoxA remains a stop with bounded raw-text observation. Physical 78, the next scene's pixels and an interactive menu are pending. See [scope](docs/STATUS_ITERATION78.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION78.md), [research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The default build configuration is RelWithDebInfo (-O2, symbols); an explicit CMAKE_BUILD_TYPE remains supported. Fast-math and floating-point contraction are disabled for the Vita application. The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration78-resource-lifetime-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 78 Resource Lifetime**, package version 01.82, on the same Vita used previously. The screen must show ITERATION 78.
4. Allow up to 100 seconds for the bounded run (120-second watchdog), then photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration78.log, iteration78-runtime.log and iteration78-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration78-resource-lifetime-r1 and include the game's log.txt and a screenshot. Inspect startup_d3d8_resource_destroyed, startup_d3d8_allocation_denied, startup_messagebox_text/caption, native scanout confirmation, startup_frame_logo_age, startup_frame_scene, startup_frame_phase and preserved context; then present=executed and the final startup_stop_import/EIP. It can present up to 240 real frames, then stops before the 241st Present; the time cap or an unsupported service can stop it sooner. There is no iteration78-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
