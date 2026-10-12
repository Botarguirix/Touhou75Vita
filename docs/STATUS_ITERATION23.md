# Iteration 23: single-thread critical-section operations

Hardware iteration 22 queried and converted its environment successfully,
freed the Unicode block, and reached EnterCriticalSection(0x0068E420) at
return VA 0x00646753. There were 42 serviced startup calls, three HeapAlloc
calls and two successful conversions. Full game boot remains unverified.

Iteration 23 supports EnterCriticalSection, TryEnterCriticalSection,
LeaveCriticalSection and DeleteCriticalSection for objects initialized by
this runtime. InitializeCriticalSection is also admitted with zero spin count.
Ownership comes from the current TEB thread ID; recursive acquisition needs
matching releases. The guest x86 state is written and read back after changes.

This model supports only the current single guest thread. Enter requiring a
wait for another owner is an unsupported diagnostic boundary; TryEnter in
that case returns FALSE without mutation. Invalid ownership, uninitialized
objects, inconsistent counters, recursion overflow and deletion while owned
are rejected. There is no scheduler or cross-thread wait implementation.

Tests cover ordinary and recursive acquisition, balanced release, foreign
ownership, nonblocking try, delete/reinitialize, invalid release and counter
corruption. Previous SEH, TLS, process, environment and conversion tests remain.
Host tests exercise production services with a memory/register double; Vita
execution is still needed for end-to-end validation.

Install 01.26 on the same Vita. Logs: iteration23.log, iteration23-runtime.log
and iteration23-watchdog.log. Expected evidence: startup_sync_state_readback=passed,
Enter and Leave serviced, and a later import boundary.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-entercriticalsection
- https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-leavecriticalsection
