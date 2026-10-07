# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Hardware iteration 50 validated the owned x86 IDirect3D8 COM root and reached the original GetDeviceCaps query in 5.5 seconds. The native renderer and original visual game boot remain pending. A fresh original Windows run reached the complete menu, and the new reference pass extracted/verified all 286 archive entries and matched 225 of 226 recorded texture uploads to decoded system resources. It also verified the original 24-bit black color key. See [reference pass 51](docs/ORIGINAL_REFERENCE_PASS_51.md), [Iteration 50 scope](docs/STATUS_ITERATION50.md) and [roadmap](docs/ROADMAP.md). The current Vita package remains iteration50/version01.54; reference pass 51 produces research artifacts and improved extraction/verification tools.

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration50-boot-startup-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe and th075.dat alongside it for the optional native title-resource preview.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 50 Boot Startup**, package version 01.54, on the same Vita used previously. The screen must show ITERATION 50.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Inspect iteration50.log, iteration50-runtime.log and iteration50-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration50-boot-startup-r1, inspect startup_d3d8_root/vtable_readback/abi and the final startup_stop_import/EIP. Include the game's log.txt. Expected next boundary is IDirect3D8::GetDeviceCaps. No graphics device or title-screen rendering is implemented in this iteration.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 50 verifies the known Japanese EXE hash before PE loading; th075.dat is read separately for a native resource preview. Local LiveArea artwork stays outside Git; public CI packages omit that art.
