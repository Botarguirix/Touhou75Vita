# Iteration 05 — Vita diagnostic VPK

## Goal

Provide a safe, installable Vita-side checkpoint before attempting to execute the translated guest code.

## Implemented in source

- Replaced the hard-coded one-shot dispatch from an empty guest-memory buffer with a diagnostic entry point.
- The app creates `ux0:data/TH075Vita/iteration01.log` and probes `ux0:data/TH075Vita/TH075.exe`.
- The probe validates the DOS and PE signatures, PE32 optional-header type, x86 machine value, section count, entry RVA, image base, and subsystem.
- Removed the large generated guest translation and semantic runtime from this diagnostic VPK target. They remain in the repository for offline analysis.
- Corrected the CMake packaging path to use VitaSDK's `vita.cmake` helpers, and enabled the build workflow on pull requests.

## Expected result

When no executable is supplied, the log reports `game_executable_missing`. With the supported x86 PE32 executable, it reports `recognized_x86_pe32` and records the header fields. The app then exits to LiveArea. Both results confirm only app startup, memory-card file access, and PE-header inspection; this build does not load sections, resolve imports, or execute the game.

## Verification status

- Source review: pending final toolchain build.
- VitaSDK build: not run; VitaSDK is not installed in the current environment.
- VPK install and hardware run: pending a build artifact and console test.

Do not mark this iteration as booting Touhou 7.5. The next milestone is a real PE loader and an import-bridge inventory, followed by an x86 execution engine decision.
