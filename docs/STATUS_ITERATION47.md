# Iteration 47: bounded continuation of original cosine initialization

## Hardware evidence from 46

Original WM_PAINT/UpdateWindow and CoInitialize completed. Main created its
event and reached EIP 0064175D inside the original CRT cos implementation.
Startup consumed 11443196 us, reported budget_exhausted and disarmed its
60-second watchdog. The displayed CreateThread was a stale earlier boundary,
not the final stopping operation. No visual game boot has been confirmed.

The known Japanese EXE's routine at 0041CAE0 initializes 3600 floats at
006884C0, calling cos at 00641754. Its main return address is 00602D0F.
This is original startup work, not an unimplemented Win32 import.

## Changes

- Correct the budget description: the current Box86 backend charges block
  entries at approximately eight guest instructions per entry; a 65536
  slice requests 8192 block entries, not 512. This is not an exact instruction
  count or a timing measurement.
- At slice expiry, walk at most 64 ascending, aligned, bounded stack frames.
  Resume only when the original cosine initializer's return address and
  loop index (0 through 3599) are found. Other exhausted locations stop.
- Log the loop index, EIP, elapsed time and the first/last completed table
  entry as float bit patterns before each continuation. The original CPU
  emulator retains registers, stack, flags and x87 state across run calls.
  No cosine values are supplied by native code and no guest code is patched.
- Allow at most 16 additional slices across the entire startup. Stop on a
  regressing index, two consecutive slices without index advancement, or
  45 seconds elapsed before another slice. The independent 60-second
  watchdog remains armed; a single translated-block loop can still evade
  the backend's slice preemption and trigger that watchdog.
- CPU budget/fault stops show the current EIP and overwrite the old API
  boundary in the log. The 300-case synthetic batch remains disabled.

Package version 01.51, title ID T075VITA1,
build_id=iteration47-boot-startup-r1. Local LiveArea art carries iteration 47
and version 01.51, using the installed-compatible 8-bit indexed PNG profile.
Proprietary artwork and game binaries remain outside the public repository.

## Hardware run and remaining work

Install the local iteration47 VPK and send iteration47.log,
iteration47-runtime.log and iteration47-watchdog.log. The progress records
will distinguish finite initialization work from a stalled cosine path.
A missing progress record means the bounded frame observer could not identify
that initializer; it is not evidence that the table finished.

Completion must be established by reaching a later original startup boundary.
File/resource loading and original Direct3D device/texture/draw/presentation
implementation still need to be reached and validated. The existing DAT
thumbnail and LiveArea title art are previews, not rendered game frames.
This iteration does not add a renderer or claim a working title screen.

## Build/package evidence

VitaSDK compilation completed. The local VPK ZIP CRC check passed; its
param.sfo contains APP_VER=01.51 and TITLE_ID=T075VITA1. All four packaged
PNG assets use the required 8-bit indexed, noninterlaced format. The generated
LiveArea version label was inspected. These are host/package checks;
continuation and x87 behavior still require the next physical Vita run.
