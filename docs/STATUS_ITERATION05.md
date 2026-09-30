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

- GitHub Actions built and published the diagnostic VPK artifact for PR #1.
- The user installed and launched that VPK on a physical PS Vita.
- The supplied hardware log reports a 2,576,384-byte I386 PE32 executable, five sections, preferred base `0x00400000`, subsystem 2, and `recognized_x86_pe32`.
- `execution=not_attempted`: this confirms app startup, file access, and header inspection only. Touhou 7.5 did not boot.
- The reverse-engineering package's recorded SHA-256 conflicts with the hash computed from its embedded EXE. The Vita log does not yet contain a hash, so exact byte identity remains unverified.

The next milestone is a loader-only WinVita proof: map the supplied PE at its preferred base, enumerate import-resolution outcomes, and stop before calling the entry point. See [PORTING_ARCHITECTURE.md](PORTING_ARCHITECTURE.md).
