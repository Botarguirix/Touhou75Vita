# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 71 passed SetRect and texture binding, then captured the original logo's first DrawPrimitiveUP: a 640×480 quad sampled from a 1024×512 texture. Random DAT container/frame selection also worked on Vita. Iteration 72/version 01.76 implements a bounded point-sampled quad renderer and displays its first changed output as an initial diagnostic snapshot. Native music remains available. Guest playback/mixing, original Present, complete rendering and gameplay remain pending. See [scope](docs/STATUS_ITERATION72.md), [external research](docs/RESEARCH_N0ZOM1Z0_ITERATION71.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration72-dat-browser-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with th075.dat and th075bgm.dat alongside it for the native resource preview and music.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 72 First EXE Quad**, package version 01.76, on the same Vita used previously. The screen must show ITERATION 72.
4. Wait for the results screen and photograph its initial EXE DRAW CAPTURE before pressing Circle. Circle picks a random graphic container and frame; Left/Right select adjacent frames. Start switches to the current DAT view and exports its DAT/BMP. Triangle selects another song, Square restarts it, and X exits.
5. Inspect iteration72.log, iteration72-runtime.log and iteration72-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration72-first-exe-quad-r1 and include iteration72-bgm.log, the game's log.txt and a screenshot. Inspect startup_d3d8_draw, draw_hash, draw_probe, draw_capture and the final startup_stop_import/EIP. Unsupported draws preserve their boundary. The first snapshot shows stored RGB; it does not establish original Present or complete game boot. For Circle include dat_view_random/index/timing/frame.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 50 verifies the known Japanese EXE hash before PE loading; th075.dat is read separately for a native resource preview. Local LiveArea artwork stays outside Git; public CI packages omit that art.
