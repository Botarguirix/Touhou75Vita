# Iteration 48: continue original resource startup across CPU slices

## Hardware evidence from 47

The original cosine index advanced from 1345 to 2710 of 3600. Two resumptions
then reached subsequent main initialization, including CreateFileA(th075.dat)
at return 0041D55D, a two-byte ReadFile and a 23220-byte ReadFile. This matches
the 215-entry archive directory (215 times 108 bytes). The original EXE,
not merely the native DAT thumbnail reader, now accesses the archive.

HeapAlloc calls reached 62, with many 116-byte resource structures after the
directory read. Main stopped at EIP 00645414, ESP 009FEAB8, EBP 009FEAE0:
the CRT SEH frame setup helper, not an exception dispatch or unknown import.
Elapsed time was 15435128 us. The watchdog disarmed. Resource decoding by
the EXE and an original rendered game frame remain unverified.

## Changes

Iteration 47 resumed only when it found the known cosine initializer on
the stack. That restriction prevented later finite initialization work from
continuing after its slice expired. Iteration 48 resumes budget expiration
inside original executable PE sections, provided the guest stack is readable
and bounded and the current TIB remains the owned main TIB. Executable
section ranges are read from the verified EXE headers. It does not resume
CPU faults, unknown imports, failed service contracts or trap returns.

Each expiration logs all general registers, EIP, EFLAGS, stack top, import
and allocation counts, elapsed time and a fingerprint of those values plus
128 stack bytes. Three consecutive identical fingerprints stop continuation;
this is a conservative sampled-state guard, not proof that a changing state
is making useful progress. The explicit cosine index guard remains active
while the known initializer is found.

All startup phases share a cap of 16 additional 65536-unit slices (8192
block entries per slice). No additional slice starts after 45 seconds.
The independent 60-second watchdog remains armed for a translated loop that
cannot be preempted. CPU state, including x87 state, remains in the same
emulator. No guest instructions, tables or archive contents are replaced.
The 300-case synthetic batch remains disabled.

## Package and next hardware run

Version 01.52 / T075VITA1 / build_id=iteration48-boot-startup-r1.
The local VPK retains the original icon and LiveArea game artwork with the
48 / 01.52 label and indexed 8-bit PNG encoding. Proprietary data and art
remain outside the public repository.

Send iteration48.log, iteration48-runtime.log and iteration48-watchdog.log.
The goal is to finish original archive-directory initialization and reach
the next actual import/resource boundary. An exhausted budget still records
the actual EIP and the reason continuation stopped. Direct3D interface
ownership, texture/draw submission and Vita presentation remain necessary
before the original title screen can be claimed. LiveArea art and the DAT
thumbnail do not establish game boot.

Host VitaSDK compilation succeeded. Package inspection confirmed ZIP CRCs,
APP_VER=01.52, TITLE_ID=T075VITA1 and four 8-bit indexed, noninterlaced PNG
assets. The 48/01.52 LiveArea label was visually inspected. The new runtime
continuations still need validation on the physical Vita.
