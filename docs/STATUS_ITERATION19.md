# Iteration 19: CRT TLS fallback under the XP compatibility profile

Hardware iteration 18 resolved InitializeCriticalSectionAndSpinCount and
called it 14 times, with successful guest-memory readback and ABI checks.
It then reached GetProcAddress("FlsAlloc"). The original instructions at
0x00646389 query four FLS exports and select existing TLS imports when those
exports are unavailable.

Iteration 19 returns NULL and ERROR_PROC_NOT_FOUND only for FlsAlloc,
FlsGetValue, FlsSetValue and FlsFree. Unknown exports remain diagnostic stops.
This selects the original CRT fallback; it does not patch the EXE.

TlsAlloc, TlsGetValue, TlsSetValue and TlsFree use 64 tracked slots in the
diagnostic thread's TEB at offset 0xE10. Allocation zeroes each slot; freeing
clears it, and invalid indices/exhaustion report an error. This remains a
single-guest-thread implementation, not a scheduler or FLS implementation.
GetLastError, SetLastError and GetCurrentThreadId read/write the same TEB.

The stdcall frame length now derives from the actual service signature:
TlsAlloc consumes 4 bytes (return address only), getters/free consume 8,
and setters consume 12. Host regression tests execute the production service
code against a memory/register test double to check stack canaries, TLS
roundtrips, reuse, exhaustion, invalid indices and unknown-export stops.
These tests do not execute the ARM dynarec; physical Vita validation is required.

The watchdog is 15 seconds, while the instruction budget remains 65536.
Iteration 18 used 4.27 seconds without a hang; the added services need time
for logging and further startup. No exception dispatch or game boot is claimed.

Install package 01.22. Evidence: four unavailable FLS export queries, TlsAlloc
served, followed by the next import. Logs: iteration19.log,
iteration19-runtime.log, iteration19-watchdog.log in ux0:data/TH075Vita.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/fibersapi/nf-fibersapi-flsalloc
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-tlsalloc
