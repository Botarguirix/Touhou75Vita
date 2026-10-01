# Iteration 21: handle count and owned process environment

Hardware iteration 20 passed GetStartupInfoA and the standard-handle queries.
It reached SetHandleCount(32), with return address 0x0064963D, after 35 serviced
startup calls. The original CRT then queries its command line and environment.

Iteration 21 handles SetHandleCount as the legacy NT no-op, returning the
requested number and consuming eight stack bytes including the return address.

GetEnvironmentStringsW/A and the ANSI GetEnvironmentStrings export return
owned copies of the current empty-environment profile. Each has two terminating
NUL characters, a separate heap address, and readback verification. The matching
FreeEnvironmentStringsW/A validates ownership and encoding, releases the block
and makes it reusable. Invalid or duplicate frees return FALSE and error 87.
Allocation exhaustion returns NULL with error 8; unknown APIs still stop.

Regression tests cover SetHandleCount values, empty-block terminators, multiple
owned copies, ANSI/Unicode mismatch, duplicate frees and complete stdcall frames.
Existing TLS, startup-info and SEH checks are retained. Host tests use a register
and memory double; physical Vita execution remains necessary.

Install package 01.24 on the same Vita. Logs: iteration21.log,
iteration21-runtime.log and iteration21-watchdog.log. Expected progress:
startup_handle_count_requested=32, startup_environment_readback=passed,
startup_environment_release=passed, then a later import boundary. A checkpoint
pass is not proof of full game boot.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-sethandlecount
- https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getenvironmentstringsw
- https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-freeenvironmentstringsw
