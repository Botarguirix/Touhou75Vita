# Touhou75Vita

Runtime prototype for Touhou 7.5 – Immaterial and Missing Power on PlayStation Vita, using a pinned WinVita PE32 loader and Box86-derived ARMv7 dynarec. Iteration 08 r2 passed its synthetic CPU and three diagnostic import calls on the user's physical Vita.

Iteration 11 r2 passed on the user's physical Vita: the original Japanese EXE ran from its entry point to GetVersionExA, with valid stack and SEH registration, a disarmed watchdog and a visible results screen. See [hardware evidence](docs/hardware/iteration11-r2/result-excerpt.txt).

Physical iteration 73 executed the second EXE quad, changed 219077 pixels, completed EndScene and read score.dat. It reached the original main loop at PeekMessageA; Present remained at 0/8. Iteration 74/version 01.78 implements an owned thread/window queue query so an empty queue can return to the game's frame path. Six new queue check groups and six renderer checks passed on PC. Physical validation of PeekMessageA and Present is pending. The diagnostic music player, DAT browser/exporter and independent title preview remain removed. See [scope](docs/STATUS_ITERATION74.md), [EXE checklist](docs/EXE_CHECKLIST_ITERATION74.md), [decompilation research](docs/RESEARCH_EXE_ITERATION73.md) and [roadmap](docs/ROADMAP.md).

## Build

Install VitaSDK and set VITASDK, then run from the repository root:

```sh
TARGET=vita bash third_party/winvita/build.sh
make -C vita
```

The package is vita/touhou75_vita.vpk (also in vita/build/). Actions publishes **Touhou75Vita-iteration74-exe-message-loop-vpk**.

## Test on one Vita

1. Keep your Japanese executable at ux0:data/TH075Vita/TH075.exe with its original data files, including th075.dat and th075bgm.dat, alongside it for requests made by the EXE itself.
2. Download the artifact from a successful Actions run for the new commit, extract the ZIP and install its VPK with VitaShell.
3. Launch **Touhou 7.5 Vita - Iteration 74 EXE Message Loop**, package version 01.78, on the same Vita used previously. The screen must show ITERATION 74.
4. Wait for the results screen, photograph the EXE capture and PRESENT FRAMES counter, and press X to exit. This build has no diagnostic music or sprite controls.
5. Inspect iteration74.log, iteration74-runtime.log and iteration74-watchdog.log in ux0:data/TH075Vita. Confirm build_id=iteration74-exe-message-loop-r1 and include the game's log.txt and a screenshot. Inspect startup_message_result=empty, MSG/LastError preservation and cleanup=24; then present_request/rect/native, present=executed, COM cleanup=24 and the final startup_stop_import/EIP. It can present up to eight real frames, then stops before the ninth Present; unsupported earlier services may stop it sooner. There is no iteration74-bgm.log.

The screen shows preflight and original-startup checkpoint results. Startup and heap services are handled under restricted contracts; unsupported imports stop execution with a diagnostic. Full game startup, a complete graphics backend, guest controls and guest audio remain future work. Other Vitas will be used after the EXE boots.

## Keep game data outside Git

Do not commit TH075.exe, TH075E.exe, translation DLLs, th075.dat, th075bgm.dat, decompiler output or other proprietary files. Supply your own files on the Vita. The known Japanese EXE hash is checked before PE loading. Local LiveArea artwork stays outside Git; public CI packages omit that art. Ghidra analysis/export outputs belong in the external artifacts directory.
