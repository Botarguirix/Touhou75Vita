# Iteration 33: repeat executable handle query before game entry

Hardware iteration 32 served GetSystemTimeAsFileTime with RTC rc=0 and
eight-byte output readback, GetCurrentProcessId=4, GetCurrentThreadId,
GetTickCount and QueryPerformanceCounter with readback. QueryPerformanceFrequency
was not called, so its implementation remains pending hardware coverage.
The run counted 110 services in 24,037,991 us, with no execution limit hit
and a disarmed watchdog. It then failed at GetModuleHandleA(NULL), return
VA 0x006424AA, because the service required a single call at 0x006423A1.

Iteration 33 removes the call-count and call-site restriction for a NULL module
name. Each NULL query returns the loaded executable base after validating its
MZ and PE headers. Named kernel32 aliases and unsupported module boundaries
remain as before. The common stdcall checks still validate stack cleanup and
callee-saved registers. This corrects the compatibility layer; guest EXE bytes
are not patched and its initialization is not skipped.

The original disassembly shows push eax at 0x006424AA followed by call
0x00602A60 at 0x006424AB. A marker records that this call is next after the
module service returns, but does not claim the target was executed. Subsequent
imports or faults must establish the actual path. A diagnostic PASS likewise
does not certify game boot, rendering or gameplay.

Install version 01.37 and confirm ITERATION 33 / GAME ENTRY with
build_id=iteration33-game-entry-startup-r1. Read iteration33.log,
iteration33-runtime.log and iteration33-watchdog.log for the repeated query,
the next startup_stop_import or fault, and startup_limit_hit. The existing
30-second watchdog remains; the previous 24-second run may approach that limit
on the next path and any timeout must be investigated separately.

VitaSDK compilation is performed; no automated tests are added or run.
Full game boot remains unverified.

Reference:
- https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea
