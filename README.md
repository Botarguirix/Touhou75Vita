# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Hardware iteration 44 completed original window creation callbacks, passed all 300 checks and stopped at ShowWindow. Iteration 45 adds visibility callbacks, native DAT title-image decoding and a resource preview. The local VPK includes original-icon bubble art and a title-screen-based LiveArea with iteration/version. Hardware validation and Direct3D rendering remain pending. A Windows Direct3D 8 capture and a read-only inventory of the user's archives are available locally. See [Iteration 45 scope](docs/STATUS_ITERATION43.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration45-batch-checks-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe and th075.dat alongside it for the optional native title-resource preview.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 45 Batch Checks**, package version 01.49, on the same Vita used previously. The screen must show ITERATION 45.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Inspect iteration45.log, iteration45-runtime.log and iteration45-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration45-batch-checks-r2, dat_title_result=decoded_original_archive_frame, screen_original_dat_title, startup_window_show=guest_callbacks_completed and the final startup_stop_import/thread. The retained 300 synthetic service cases still report individually. The original EXE icon in the corner is a decoded resource preview; visual game boot remains unverified.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 45 verifies the known Japanese EXE hash before PE loading; th075.dat is read separately for a native resource preview. Local LiveArea artwork stays outside Git; public CI packages omit that art.
