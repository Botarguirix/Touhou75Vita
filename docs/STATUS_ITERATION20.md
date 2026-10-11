# Iteration 20: GUI process startup information

Hardware iteration 19 passed the original TLS fallback and two HeapAlloc
calls, returning buffers at 0x00C00000 and 0x00C00090. It stopped at
GetStartupInfoA (return VA 0x006494AC, output buffer 0x009FEEA0).

Iteration 20 supplies a 68-byte x86 STARTUPINFOA for a GUI process without
inherited CRT file handles or a console. Reserved fields and flags are zero;
cb is 68. The output is read back before returning with an 8-byte stack
cleanup. Invalid or truncated buffers cause a diagnostic contract failure.

GetStdHandle returns NULL for the three standard selectors under the explicit
no-console profile; invalid selectors return INVALID_HANDLE_VALUE with an
error. GetFileType handles only absent/invalid standard handles, reporting
FILE_TYPE_UNKNOWN plus ERROR_INVALID_HANDLE. Other handles remain unsupported.
GetCommandLineA returns the persistent command line already owned by the
diagnostic process context. Its zero-argument return consumes 4 stack bytes.

Host tests use the production startup-services source and verify structure
size, guard words around the output, invalid pointers, standard selectors,
and persistent command-line bytes, as well as previous TLS/SEH regressions.
These tests do not prove ARM execution or a full game boot.

Install version 01.23 on the same Vita. Logs: iteration20.log,
iteration20-runtime.log and iteration20-watchdog.log. Evidence sought:
startup_info_readback=passed and a later import boundary.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa
- https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getcommandlinea
