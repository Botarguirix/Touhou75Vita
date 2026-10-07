# Iteration 39: ten service checks in one launch

Hardware iteration 38 reported batch_passed=4, batch_failed=0. It covered
auto-reset and manual-reset events, moving/zeroing/shrinking heap allocations,
and UTC/QPC output with preserved LastError. The original EXE still stops
at SetThreadPriority. This iteration implements the user's request for ten
checks per VPK, rather than ten separately installed packages.

Tests 01..04 retain those checks. Added cases:

05. A closed event rejects SetEvent and WaitForSingleObject with invalid-handle
    error 6, returning FALSE and WAIT_FAILED respectively.
06. In-place-only heap growth that exceeds capacity fails while preserving the
    original pointer's size, contents and LastError. The allocation is freed.
07. A fresh TLS slot reads zero, stores/returns a value, frees successfully and
    then rejects a get on the freed index with error 87. It uses a newly allocated
    slot, not the original game's slot.
08. CP932 bytes 82 A0 41 00 decode to U+3042, U+0041, NUL; the size query and
    reverse conversion agree and the used-default-character flag is false.
09. SetLastError/GetLastError match the guest TEB field; GetCurrentThreadId=8
    leaves that error intact in the restored main context.
10. GetModuleFileNameA with capacity 4 writes only four pathname characters,
    preserving following guard bytes; sufficient capacity returns the full
    pathname length with its terminating NUL.

Every case gets batch_test_NN_name/result/error and batch_test_begin markers.
A failed case does not skip later cases. Summary: batch_total=10, batch_passed,
batch_failed, batch_result. The screen derives N/10 from the log rather than
a hardcoded denominator. Overall success requires the original startup
checkpoint and all ten checks. Their hardware outcome is pending, not assumed
from successful compilation.

The batch uses the same StartupServices via synthetic guest stdcall frames,
after the original startup watchdog ends. It does not run the original game
instructions for those synthetic API calls. CPU context, stack frame bytes
and main LastError are restored; temporary event/heap/TLS resources are
released. The bounded heap cursor advances because freed space is not yet
recycled. Only zero-time event polls are exercised; no blocking waits are
introduced. Startup counters precede batch records and must not be read as
calls made by the original EXE. The EXE still stops at SetThreadPriority;
scheduler priorities, event/time wakeups and full visual boot remain pending.

Install 01.43, confirm ITERATION 39 / BATCH CHECKS and
build_id=iteration39-batch-checks-r1. Logs: iteration39.log,
iteration39-runtime.log, iteration39-watchdog.log. VitaSDK compilation is
performed; the ten runtime checks execute on the user's Vita when launched.
