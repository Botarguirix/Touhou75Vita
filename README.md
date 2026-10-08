# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 70 passed ZWRITEENABLE and source/destination blend setters, then reached USER32!SetRect before a textured quad. Iteration 71/version 01.75 prepares the signed RECT contract so the existing first-draw diagnostics can capture that geometry if reached. Circle now selects a random graphic container and a random frame, avoiding the previous container. Native music remains available. Guest playback/mixing, original rasterization/Present and gameplay remain pending. See [scope](docs/STATUS_ITERATION71.md), [updated external research](docs/RESEARCH_N0ZOM1Z0_ITERATION71.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration71-dat-browser-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with th075.dat and th075bgm.dat alongside it for the native resource preview and music.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 71 Random DAT Rect**, package version 01.75, on the same Vita used previously. The screen must show ITERATION 71.
4. Wait for the results screen. Circle picks a random graphic container and frame; Left/Right select adjacent frames, and Start exports DAT/BMP. Triangle selects another song, Square restarts it, and X exits. Photograph the screen.
5. Inspect iteration71.log, iteration71-runtime.log and iteration71-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration71-random-dat-rect-r1 and include iteration71-bgm.log, the game's log.txt and a screenshot. Inspect startup_rect_set/readback, texture_binding, draw_boundary/draw_state/draw_stage/vertex and the final startup_stop_import/EIP. DrawPrimitiveUP is captured but not executed. For Circle include dat_view_random/index/timing/frame. Unsupported graphics, Win32, input or audio services may be the next boundary. Title-screen drawing is not implemented yet.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 50 verifies the known Japanese EXE hash before PE loading; th075.dat is read separately for a native resource preview. Local LiveArea artwork stays outside Git; public CI packages omit that art.
