# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 10 passed identity, CPU, IAT, heap, file, TEB/FS, dynamic TLS, basic process and visible-screen checks on the user's physical Vita. It uses a single diagnostic TEB/PEB and partial Win32 APIs. The game's entry point, TLS callbacks and English patch are not executed. See [scope and hardware results](docs/STATUS_ITERATION10.md) and the [next startup probe](docs/STARTUP_PROBE_PLAN.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk. Actions builds pushes to main and codex/d2vita-runtime-review and publishes **Touhou75Vita-iteration10-teb-fs-tls-vpk**.

## Test on Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 10 Thread Context Test**, package version 01.12.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Share ux0:data/TH075Vita/iteration10.log and iteration10-runtime.log. Confirm build_id=iteration10-winvita-teb-fs-tls-r1. Expected services result: identity_services_teb_tls_process_passed. Display reports screen_result=presented separately.

The screen shows diagnostic results. Game startup, Direct3D 8 graphics, controls and audio remain future work.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 10 needs only the known Japanese EXE and verifies its hash before PE loading.
