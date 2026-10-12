# Iteration 49: original game log file writes

## Hardware evidence from 48

Four bounded resumptions reached the next original import without a CPU
limit or fault. Main read directory headers and bodies from th075.dat
(23220 bytes, 215 entries), th075bgm.dat (3672 bytes, 34 entries) and
th075b.dat (3996 bytes, 37 entries). HeapAlloc calls reached 312. Elapsed
time was 35140570 us; the watchdog disarmed.

The stopping CreateFileA names log.txt, return 0041CC32. Static inspection
of this call shows GENERIC_WRITE, share zero, null security/template,
CREATE_ALWAYS and FILE_ATTRIBUTE_NORMAL. The adjacent original logging
routine reopens it with OPEN_EXISTING, seeks to EOF, calls WriteFile and
closes the handle. The previous read-only service deliberately stopped here.

## Changes

- Serve the observed write-only CreateFileA contract only for normalized
  ux0:data/TH075Vita/log.txt. CREATE_ALWAYS truncates/creates; OPEN_EXISTING
  reopens without truncation. Creation reports the appropriate existing/new
  LastError. Resource archives retain their read-only opening contract.
- Track writable owned handles separately. WriteFile copies a validated
  synchronous guest buffer, writes it at the current file position, flushes
  it, reports actual bytes and propagates errors. Requests above 4 MiB and
  overlapped requests remain unsupported. Closed/unknown handles fail;
  archive handles deny writing and write-only log handles deny reading.
- Existing SetFilePointer provides the original EOF seek. CloseHandle
  closes the stream and clears ownership/access state. Destruction closes
  any remaining streams.
- Log access/disposition/share/flags, write scope, requested/actual bytes
  and flush result. Buffer runtime diagnostics in 8 KiB and flush each
  main import boundary, before continuation and before displaying the final
  results. The watchdog retains its separate unbuffered file. This reduces
  per-line native writes; speed improvement needs hardware measurement.

No batch tests, dummy file handles or simulated successful writes are added.
The original game may recreate log.txt on startup as it does on Windows.
The bounded continuation and 60-second watchdog from 48 remain active.

## Package and hardware run

Version 01.53 / T075VITA1 / build_id=iteration49-boot-startup-r1.
LiveArea keeps the original icon and game artwork with label 49 / 01.53.
PNG assets retain the compatible 8-bit indexed format.

Install iteration49 and send iteration49.log, iteration49-runtime.log,
iteration49-watchdog.log and the game's log.txt if present. The next
checkpoint is returning from original logging and recording the following
actual resource/graphics import. The original Direct3D rendering backend
and visual game boot remain pending; the DAT preview is not a game frame.

VitaSDK compilation and local VPK inspection succeeded: ZIP CRCs,
APP_VER=01.53, TITLE_ID=T075VITA1 and all four PNG encoding profiles passed.
LiveArea's iteration/version label was inspected. Runtime behavior and
diagnostic timing still require the next physical Vita run.

Contract references:
- [CreateFileA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea)
- [WriteFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile)
