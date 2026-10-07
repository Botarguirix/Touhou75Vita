# Iteration 41: directory mount, window dimensions and 100 service cases

## Hardware evidence from iteration 40

The supplied logs report 20/20 passed. The worker resumed after 330026 us with
real_16ms_timeout, executed original instructions and reblocked at its next
WaitForSingleObject. Main resumed and stopped at SetCurrentDirectoryA,
return address 0x00602ADD. Startup elapsed time was 25733462 us.

## Original EXE progress in this build

SetCurrentDirectoryA accepts the one mounted directory C:\TH075, including
ASCII case differences, forward separators, a trailing separator and `.`.
It verifies ux0:data/TH075Vita with sceIoGetstat before returning success.
The mapping is shared by main and worker. No native process chdir is performed.
Other directory paths stop with an explicit unmounted_path boundary.
GetCurrentDirectoryA returns the canonical guest directory; short buffers
receive the required size including NUL without output writes. Successful
operations preserve LastError. Guest writes are read back and stdcall is checked.
Relative file opening and other mounts still require their own implementation.

The original instructions next query USER32 GetSystemMetrics indices
45,5,7,46,6,8,4 to calculate their window size. These are supported under a
logical borderless 640x480 desktop with zero border/caption dimensions.
Screen width/height indices 0/1 expose that same logical desktop. This profile
is a configuration for the future renderer; it is not a measured Vita panel
size or evidence of an existing game window. Other metrics remain unsupported.
Resource loading, window registration and real graphics must be implemented
against subsequent observed calls. Unknown calls remain diagnostic boundaries.

## Batch: 100 individually identified cases

Cases run sequentially after original startup and after its watchdog is disarmed.
The batch retains the main CPU/frame/LastError restore and resource cleanup.
The shared service ABI validates return registers, stack cleanup and preserved
registers on every synthetic call. Each case logs begin, parameters, unique
name, result and failure description. All independent cases are attempted.

| IDs | Cases |
| --- | --- |
| 01–20 | Existing event, heap, clock, TLS, priority, TEB and CP932 checks |
| 21–40 | Directory aliases with capacities 0–19, NUL/length, output guards and LastError |
| 41–60 | Zeroed heap allocations across 20 sizes around alignment boundaries, exact size and cleanup |
| 61–80 | 20 distinct CP932 characters: control/ASCII, halfwidth punctuation and hiragana; explicit-length size query, roundtrip, guards and default flag |
| 81–89 | Nine logical desktop metrics, return values and preserved LastError |
| 90–100 | Auto/manual events across 1–11 signal/wait/reset cycles and cleanup |

These are parameterized contract tests, including repeated event stress cycles.
Their count is separate from original game milestones. They cannot establish
asset loading, graphics initialization or the actual title screen. Hardware
execution of iteration 41 remains pending even when VitaSDK compilation succeeds.

Install package version 01.45, title ID T075VITA1. Confirm ITERATION 41,
build_id=iteration41-batch-checks-r1 and batch_total=100. Collect iteration41.log,
iteration41-runtime.log and iteration41-watchdog.log. Prioritize the final
startup_stop_import/thread and original directory/metric calls in the next fix.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
- https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetrics
