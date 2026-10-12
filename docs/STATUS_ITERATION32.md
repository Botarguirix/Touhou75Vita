# Iteration 32: clocks for CRT startup

Hardware iteration 31 returned HeapSize=128 for block 0x00C00780,
released its critical section and reached GetSystemTimeAsFileTime at return
VA 0x00645259. It counted 75 services in 15,986,260 us, without a limit hit;
the watchdog disarmed. Correcting HeapSize changed the path: this run did
not call HeapReAlloc, so its implementation is still pending hardware coverage.

The original routine at 0x00645238 reads time, process ID, thread ID, a tick
count and a performance counter to initialize CRT state. Iteration 32 handles
GetSystemTimeAsFileTime, GetCurrentProcessId, GetTickCount,
QueryPerformanceCounter and QueryPerformanceFrequency together.
GetCurrentThreadId was already handled from the guest TEB.

FILETIME comes from the Vita UTC RTC, converted through sceRtcSetTick and
sceRtcGetWin32FileTime. RTC failures stop explicitly; no fabricated timestamp
is returned. The output is eight little-endian bytes with readback checking.
The VOID API does not advertise a success BOOL. QPC uses the native process
uptime in microseconds; QPF reports 1,000,000 counts per second. Both use
eight-byte outputs and return TRUE after a successful write. GetTickCount
returns the low 32 bits of uptime in milliseconds. Its epoch is deliberately
virtualized to native process creation, rather than the physical console's
boot time. Guest process ID is read from TEB+0x20, consistent with the existing
single-process profile; no host process identifier leaks into the guest.
Services preserve LastError and use the common stdcall ABI checks.

The new clock counter is included in startup_serviced_imports. Invalid output
buffers are diagnostic contract failures; guest access violations and exception
dispatch are not emulated by these services. The 30-second watchdog remains.
This is CRT initialization support, not confirmation of game initialization,
asset loading or rendering. Full game boot remains unverified.

Install version 01.36; confirm ITERATION 32 and
build_id=iteration32-clock-startup-r1. Logs: iteration32.log,
iteration32-runtime.log, iteration32-watchdog.log. Look for clock readbacks,
startup_guest_process_id and the new startup_stop_import. VitaSDK compilation
is performed; no automated tests are added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemtimeasfiletime
- https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
- https://docs.vitasdk.org/psp2_2rtc_8h_source.html
