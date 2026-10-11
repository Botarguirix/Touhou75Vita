# Iteration 36: original worker first-import diagnostic

Hardware iteration 35 created auto-reset, initially nonsignaled event
0x00AB2000 and reached CreateThread at return 0x00423A58. It counted 113
services in 24,321,723 us, without a limit hit; the watchdog disarmed.
The original code requests entry 0x00423B40, NULL parameter, default stack,
no creation flags and a thread-ID output at 0x0068BE3C.

Iteration 36 leaves the main CreateThread call unserviced. After the main
stops there, it validates all six observed arguments, saves the full CPU
context including FPU/SIMD, and executes a diagnostic slice of that original
worker function. Stack: 0x00A00000..0x00A20000, TEB: 0x00760000. The TEB has
an empty SEH chain, stack bounds, self pointer, guest process ID 4, diagnostic
thread ID 12, shared PEB and zero TLS/LastError state. The entry stack holds
a diagnostic return sentinel and the original parameter. Guest process state
and image are shared; memory writes from the diagnostic are not rolled back.

Execution stops at the first intercepted import without executing its API.
The import name, return address and bounded worker stack are recorded. The
worker budget is 4096 approximate instructions; the existing 30-second startup
watchdog covers both main and worker phases. startup_elapsed_us now includes
the worker probe; worker_probe_elapsed_us is reported separately. The main
context and current-TIB binding are restored afterward. No thread handle or
thread-ID output is fabricated, and the main does not resume beyond CreateThread.

This diagnostic identifies the worker's immediate timing/synchronization needs
before integrating real context scheduling, wakeups, thread handles and priority.
It is not a full CreateThread implementation or visual boot. A worker probe
fault/limit causes the overall diagnostic to fail, with worker-specific logs.

Install 01.40, confirm ITERATION 36 / WORKER PROBE and
build_id=iteration36-worker-probe-r1. Inspect worker_create_arg0..5,
worker_probe_import, worker_probe_frame, worker_probe_result and watchdog.
Logs: iteration36.log, iteration36-runtime.log, iteration36-watchdog.log.
VitaSDK compilation is performed; no automated tests are added or run.
