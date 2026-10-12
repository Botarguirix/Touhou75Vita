# Iteration 28: original executable module path

Hardware iteration 27 completed five LCMapStringW calls, ten conversion calls
and balanced Enter/LeaveCriticalSection. The final section state had recursion
and owner zero, with readback passing. It recorded 62 counted startup calls
and stopped at GetModuleFileNameA, return VA 0x006492B5. Elapsed original
startup time was 13,397,116 us; the 15-second watchdog was disarmed.

Disassembly shows GetModuleFileNameA(NULL, 0x0068E578, 260). Iteration 28
returns the virtual guest pathname C:\TH075\TH075.exe, consistent with the
existing process command line and parameters. This is a guest path, not a
host file operation. Actual game files remain external on ux0:data/TH075Vita.

NULL and the loaded PE base identify the original executable. Other module
handles stop explicitly rather than inventing DLL filenames. A sufficient
buffer receives a NUL-terminated path; return length excludes NUL. XP-profile
truncation returns the capacity and writes that many characters without NUL,
setting last error to zero. Zero capacity writes nothing and returns zero
with last error zero. NULL output with nonzero capacity returns error 87.
Writes are bounded to the path size and checked by readback. stdcall cleanup
is 16 bytes and preserved registers are checked by the existing service guard.

The watchdog increases to 30 seconds because the measured original-startup
run is already close to 15 seconds. The execution budget is unchanged and
the watchdog still reports/disarms normally; the increase is not a claim
that game boot will finish within 30 seconds.

Install 01.31, confirm ITERATION 28 / iteration28-module-path-startup-r1.
Logs: iteration28.log, iteration28-runtime.log, iteration28-watchdog.log.
Expected evidence: startup_module_filename_readback=passed, serviced
GetModuleFileNameA, and a later boundary. Full game boot remains unverified.
VitaSDK compilation is performed; no automated tests were added or run.

Reference:
https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
