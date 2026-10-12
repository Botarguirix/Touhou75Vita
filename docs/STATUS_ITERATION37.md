# Iteration 37: initial worker-to-main context handoff

Hardware iteration 36 executed the original worker from 0x00423B40 to
WaitForSingleObject, return 0x00423B8C, with ESP 0x00A1EFA4 in its own
stack. The worker took 96,397 us, without a budget hit. Main initialization
remained stopped at CreateThread. Total time was 24,514,120 us and the
watchdog disarmed.

Iteration 37 keeps the observed six-argument CreateThread profile: default
security/stack, entry 0x00423B40, NULL parameter, flags zero, ID output
0x0068BE3C. It runs that worker using the existing separate 128 KiB bounded
stack and fresh TEB. At its first wait it validates the frame, tracked
nonsignaled event and positive timeout, records the start time, and saves
the full CPU context at the unexecuted wait. TLS slot zero is checked empty.
The object owns the saved context, handle 0x00AB4000 and guest ID 12.

The main CPU/TIB context is restored and its registers/EIP/EFLAGS/FS are
checked. The ID output is written/read back. The CreateThread frame returns
the tracked worker handle with 28-byte stdcall cleanup, checked against
EAX/EIP/ESP. Main then resumes at 0x00423A58 using the original import hook,
with a fresh bounded instruction budget. The thread-creation count is included
in serviced imports. Only one worker and this observed call site are supported.

This is an initial cooperative handoff, not a complete scheduler. The worker's
wait frame remains saved without a synthetic timeout result. It is not woken
or rerun while main proceeds to the next unsupported import. Once that
boundary is reached, both contexts stop for diagnostics. Wait expiry/event
wakeups, priorities, thread exit/close and further thread creation remain
required before continued execution beyond these boundaries. The expected
next static import is SetThreadPriority; it remains unsupported. The stack
size is a diagnostic profile rather than full PE stack-reservation emulation.
Full visual boot remains unverified.

The existing 30-second watchdog covers initial main execution, worker and
resumed main. Install 01.41 and confirm ITERATION 37 / THREAD HANDOFF with
build_id=iteration37-thread-handoff-r1. Inspect worker_wait_state,
startup_main_context_readback, startup_thread_id_readback,
startup_thread_handoff and the latest startup_stop_import. Earlier CreateThread
stop records document the initial interception; the later stop is the final
boundary. Logs: iteration37.log, iteration37-runtime.log,
iteration37-watchdog.log. VitaSDK compilation is performed; no automated
tests are added or run.

Reference:
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
