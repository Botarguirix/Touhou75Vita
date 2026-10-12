# Iteration 29: module alias and CRT processor feature query

Hardware iteration 28 returned the executable path with output readback
passing, performed two additional HeapAlloc calls and passed the current
HeapFree bridge. It recorded 65 counted startup calls and stopped at
GetModuleHandleA("kernel32"), return VA 0x0064521A. Elapsed startup was
13,773,057 us; the 30-second watchdog disarmed and no execution limit hit.
The existing HeapFree success stub is not evidence of a complete allocator.

Iteration 29 accepts case-insensitive "kernel32" as well as "kernel32.dll",
following GetModuleHandle's default .dll extension rule for this known module.
Both return the same opaque compatibility handle, not a loaded Windows DLL.
Other names remain diagnostic boundaries.

Disassembly after this call requests GetProcAddress("IsProcessorFeaturePresent")
and calls the returned function with argument 0. The service now resolves that
export to a reserved runtime trap, separate from the existing critical-section
export and import traps. Trap capacity checks account for the lower reservation.
Its returned guest address is executable through the trap dispatcher.

IsProcessorFeaturePresent(0) returns FALSE for PF_FLOATING_POINT_PRECISION_ERRATA
under the translated CPU profile: it does not advertise the Pentium precision
erratum. This is not certification of floating-point instruction accuracy.
Other feature IDs stop explicitly; no host ARM feature claims are exposed to
the x86 guest. stdcall cleanup is eight bytes and preserved registers are checked.

Install 01.32, confirm ITERATION 29 / iteration29-cpu-feature-startup-r1.
Logs: iteration29.log, iteration29-runtime.log, iteration29-watchdog.log.
Expected evidence: kernel32 alias resolved, the export resolved/called,
startup_processor_feature_requested=0, serviced IsProcessorFeaturePresent,
and a later startup boundary. Game boot remains unverified.
VitaSDK compilation is performed; no automated tests were added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent
