# Iteration 35: unnamed events during game initialization

Hardware iteration 34 verified the timer helper's saved return 0x00602A7C
and game function's saved return 0x006424B0. It served timeBeginPeriod(1)
with a passed ABI check and reached CreateEventA at return 0x00423A3B.
Static disassembly pushes four zero arguments: default security, auto-reset,
initially nonsignaled, unnamed. The run counted 112 serviced imports in
25,542,566 us, with no limit hit and a disarmed watchdog. Game function
entry is now confirmed on hardware; visual boot is still unverified.

Iteration 35 implements CreateEventA for unnamed events with default security.
Nonzero BOOL arguments select manual reset and initial signal state. Each
object has a unique opaque tracked handle; a returned handle is not a guest
pointer. The profile allows 64 live events, with a bounded, nonrecycled handle
range per run. Resource exhaustion returns NULL and LastError=8. Named events
and custom security remain explicit unsupported boundaries.

SetEvent/ResetEvent update tracked state; CloseHandle destroys tracked events.
WaitForSingleObject succeeds immediately for signaled events, consuming an
auto-reset signal while retaining a manual-reset signal. A zero-time poll of
a nonsignaled event returns WAIT_TIMEOUT. A positive/infinite wait on an
unsignaled event stops before executing that wait: a guest scheduler is needed
to let another thread signal it. Foreign handle types remain unsupported;
closed handles in the event range fail with LastError=6 and FALSE/WAIT_FAILED.
The common stdcall ABI checks remain. Only CreateEventA is hardware-observed;
the other event operations remain pending hardware coverage.

This is single-guest-thread event state, not a threaded synchronization system.
The next static initialization steps include CreateThread and thread priority;
these remain unsupported and must be implemented with a real guest context,
stack, TEB/TLS, scheduling and synchronization. Returning an invented successful
worker-thread handle would not establish that its code executed.

Install 01.39; confirm ITERATION 35 / GAME EVENT and
build_id=iteration35-event-startup-r1. Logs: iteration35.log,
iteration35-runtime.log and iteration35-watchdog.log. Inspect event arguments,
created handle/state, startup_event_calls, the next stopped import and the
watchdog. The existing 30-second watchdog remains unchanged.
VitaSDK compilation is performed; no automated tests are added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventa
- https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
- https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
