# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 09 adds on-console SHA-256 verification, synthetic x86 heap allocation/write/free and read-only EXE access through the original IAT, plus a native results screen. Its VitaSDK build and hardware validation are pending. The game's entry point, TLS callbacks and English patch are not executed. See [scope and expected results](docs/STATUS_ITERATION09.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk. Actions builds pushes to main and codex/d2vita-runtime-review and publishes **Touhou75Vita-iteration09-heap-file-screen-vpk**.

## Test on Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 09 Services Test**, package version 01.10.
4. Wait for the results screen. Photograph it, then press X to exit; automatic exit after 120 seconds.
5. Share ux0:data/TH075Vita/iteration09.log and iteration09-runtime.log. Confirm build_id=iteration09-winvita-heap-file-screen-r1. Expected services result: identity_cpu_iat_heap_file_passed. Display reports screen_result=presented separately.

The screen shows diagnostic results. Game startup, Direct3D 8 graphics, controls and audio remain future work.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat or other proprietary files. Supply your own files on the Vita. Iteration 09 needs only the known Japanese EXE and verifies its hash before PE loading.
