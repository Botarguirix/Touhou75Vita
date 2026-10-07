# Iteration 40: priority dispatch, real timeout and ten additional checks

Hardware iteration 39 reported 10/10 passed. The actual startup boundary was
SetThreadPriority for tracked worker 0x00AB4000 at return 0x00423A6C.

Iteration 40 supports SetThreadPriority/GetThreadPriority on that worker object.
Supported relative priorities are -15, -2..2 and 15 under the normal process
profile. Unsupported values fail with error 87; foreign handles fail with error
6. Priority state is stored on the real saved worker context, not a host thread
handle. Success preserves LastError and uses the shared stdcall ABI checks.

The observed positive-priority request yields main execution at its completed
API return. The dispatcher verifies the original nonsignaled event and 16 ms
timeout, checks the native elapsed time (waiting the bounded remainder if
necessary), then returns WAIT_TIMEOUT in the saved worker frame. It executes
original worker instructions with its own stack/TEB, using existing services
where supported. On another nonsignaled blocking wait it saves/reblocks the
worker and resumes main. On an unsupported worker API it stops both contexts
and records startup_stop_thread=worker plus the import. A fault, malformed
frame or instruction/call limit fails explicitly. No signal is fabricated.
The original startup watchdog covers this additional slice.

This is one bounded priority/timeout dispatch, not a continuously preemptive
scheduler. Only the observed worker and 16 ms initial wait are supported;
more workers, arbitrary deadlines/event wakeups, exit/close and scheduling
fairness still require implementation. The worker's priority is applied to
this dispatch before main resumes; native Vita priority is not changed.
Full visual boot remains unverified. Its outcome is pending hardware logs.

Tests 01..10 from iteration 39 remain. New tests:

11. All seven supported worker priorities roundtrip and restore the original.
12. Invalid priority fails while preserving the original value.
13. Foreign thread handles reject priority get/set.
14. Worker and main LastError/ID are isolated through their TEB bindings.
15. A newly allocated TLS index has distinct values in main and worker TEBs.
16. A malformed CP932 lead byte fails strict decoding without output writes.
17. A short conversion buffer returns error 122 without writing guard bytes.
18. Zero-size heap allocation returns a valid pointer with recorded size zero.
19. Growth within aligned capacity preserves the pointer/data and zeros the tail.
20. Zero-capacity module-path query leaves its destination untouched.

Each test has name/result/error. Temporary TLS/heap/events and modified worker
TEB fields/priority are restored or released. The batch saves main CPU/frame
state and LastError. It runs after startup counters and watchdog, using direct
synthetic service frames, not original EXE calls. The screen shows N/20 and
overall success requires all twenty and the startup checkpoint. Compilation
does not imply the new runtime tests pass.

Install 01.44, confirm ITERATION 40 / BATCH CHECKS and
build_id=iteration40-batch-checks-r1. Logs: iteration40.log,
iteration40-runtime.log, iteration40-watchdog.log. Inspect actual priority
calls, worker_wake_elapsed_us/reason/result, latest startup_stop_import and
batch_test_11..20. VitaSDK compilation is performed; hardware execution is pending.

Reference:
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadpriority
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadpriority
