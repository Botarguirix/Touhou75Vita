# Iteration 25: CRT character classification

Hardware iteration 24 returned from GetACP (932), allocated another heap
block, and returned from GetCPInfo with its output readback passing. It
recorded 46 counted startup calls, four HeapAlloc calls and two code-page
calls. The next boundary was GetStringTypeW, return VA 0x0064DA32.
The execution limit was not reached and the watchdog was disarmed.

The disassembly at 0x0064DA2C calls GetStringTypeW(CT_CTYPE1,
0x006639D0, 1, stack_output). The supplied image contains a UTF-16 NUL
at that source address; this is the CRT's Unicode API availability probe.

Iteration 25 implements CT_CTYPE1 for ASCII UTF-16 units, including controls,
whitespace, blanks, letters, digits, hexadecimal digits, punctuation and
C1_DEFINED. Negative input lengths include the first terminating NUL;
positive lengths preserve embedded NULs. Input is bounded to 16384 units.
Output is prepared before writing and checked by readback. Invalid NULL,
zero length and overlapping buffers return FALSE with error 87. Unsupported
Unicode units or classification levels stop explicitly without writing
partial results. stdcall cleanup is 20 bytes; preserved registers are checked.

This is not full Unicode classification or Japanese text conversion. The
subsequent CRT path can require MultiByteToWideChar, locale or mapping APIs;
those remain explicit boundaries. Game boot and graphics remain unverified.

Install version 01.28. Confirm ITERATION 25 and
build_id=iteration25-character-startup-r1. Expected evidence is a serviced
GetStringTypeW, startup_string_type_readback=passed and the next boundary.
Logs: iteration25.log, iteration25-runtime.log, iteration25-watchdog.log.

VitaSDK compilation is performed for this iteration; no automated tests
were added or run. The physical startup result is still pending.

Reference:
https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-getstringtypew
