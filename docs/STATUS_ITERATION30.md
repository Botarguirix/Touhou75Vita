# Iteration 30: exception filter registration

Hardware iteration 29 serviced the extensionless kernel32 query, resolved
IsProcessorFeaturePresent, and returned from its feature-0 call. Two further
HeapAlloc calls completed. The run counted 70 startup calls and stopped at
SetUnhandledExceptionFilter(0x0064654C), return VA 0x006465A5. Startup elapsed
15,194,799 us; no execution limit hit and the 30-second watchdog disarmed.

Iteration 30 implements the registration/state portion of
SetUnhandledExceptionFilter. Initially no filter is installed (zero). Each
call returns the previous guest callback and stores the new one. NULL resets
the filter to default. Nonzero callbacks must be readable inside the original
loaded image; other addresses are an explicit unsupported boundary. This
check confirms address scope, not an executable-section or callback-body test.
stdcall cleanup is eight bytes and preserved registers are checked.

This is partial exception support. The registered callback is not invoked
by runtime faults; guest exception dispatch, EXCEPTION_POINTERS/CONTEXT and
unwinding are still unimplemented. Logs explicitly record registration_only
and startup_exception_dispatch=not_implemented. Native/runtime faults remain
diagnostic stops. This iteration enables normal startup past registration
and does not claim a working exception recovery mechanism.

Install revision 2, version 01.34; confirm ITERATION 30 R2 /
iteration30-exception-filter-startup-r2.
Logs: iteration30.log, iteration30-runtime.log, iteration30-watchdog.log.
Expected evidence: previous filter zero, registered pointer 0x0064654C,
SetUnhandledExceptionFilter serviced, and a later boundary. Full game boot
remains unverified. VitaSDK compilation is performed; no automated tests
were added or run.

Reference:
https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter

## Revision 2: packaging repair

The user could not install revision 1 (0x80105A03). Direct inspection of its
param.sfo found STITLE data length 56 bytes with maximum field size 52; the
declared data extended into the following field. Iteration 29 had length 51
and fit its field. This is a concrete metadata defect; the numeric installer
error alone was not used to infer its meaning.

Revision 2 shortens the title, bumps APP_VER to 01.34 and adds a CMake guard
rejecting application names over 51 bytes. The runtime service change stays
the same as revision 1. Rebuilt SFO metadata must fit every declared field;
physical installation still needs confirmation. Logs remain iteration30*.log.
