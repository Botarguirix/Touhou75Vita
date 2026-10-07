# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 54 confirmed DirectSound8 COM activation, native audio-port configuration and release, then reached the original primary CreateSoundBuffer request. Iteration 55/version 01.59 adds an audible native verification using original effect 2 from wave/se.dat inside the user's th075.dat. It plays at the results screen; Square replays it. Hardware audibility is pending. The original EXE still stops at CreateSoundBuffer; guest playback and original title-screen rendering remain pending. See [iteration 55 evidence and procedure](docs/STATUS_ITERATION55.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration55-dat-audio-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe and th075.dat alongside it for the optional native title-resource preview.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 55 DAT Audio**, package version 01.59, on the same Vita used previously. The screen must show ITERATION 55.
4. Wait for the results screen. Listen for the original effect; press Square to replay it. Photograph the screen, then press X to exit; automatic exit after 120 seconds.
5. Inspect iteration55.log, iteration55-runtime.log and iteration55-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration55-dat-audio-r1, inspect audio_probe_asset/original/output/release_rc and the final startup_stop_import/EIP. Include the game's log.txt. Inspect the REF/software device fallback and texture allocations/uploads when reached. Unsupported device methods, Win32, input or audio services may be the next boundary. Title-screen drawing is not implemented yet.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, Direct3D 8 graphics, controls and audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 50 verifies the known Japanese EXE hash before PE loading; th075.dat is read separately for a native resource preview. Local LiveArea artwork stays outside Git; public CI packages omit that art.
