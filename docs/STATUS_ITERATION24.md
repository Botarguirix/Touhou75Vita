# Iteration 24: Japanese startup code-page metadata

Physical iteration 23 executed the original EXE and recorded 43 counted startup
service calls. EnterCriticalSection returned with state readback passing.
Execution stopped at KERNEL32.dll!GetACP, return VA 0x006503FB, without hitting
the execution limit. This is still CRT startup; game boot is unverified.

Iteration 24 adds GetACP and GetOEMCP returning 932 for the explicit Japanese
compatibility profile. GetCPInfo accepts CP_ACP (0), CP_OEMCP (1) and 932,
and returns the 20-byte x86 CPINFO structure: MaxCharSize 2, default '?',
lead-byte ranges 81-9F and E0-FC, remaining bytes zero. Output memory is
checked and read back. A NULL output returns FALSE with error 87; other
code pages stop at an unsupported boundary. stdcall cleanup includes the
return address and the actual argument count; preserved registers are checked.

This supplies metadata only. Full CP932 character conversion, locale services,
exception dispatch, graphics and complete game startup are still pending.
Existing WideCharToMultiByte remains limited to its documented ASCII subset.

Install version 01.27 on the same Vita. The screen must show ITERATION 24.
Logs: iteration24.log, iteration24-runtime.log, iteration24-watchdog.log.
Look for startup_code_page_profile=japanese_cp932, a serviced GetACP and
the next stop import. GetCPInfo is available if the original CRT requests it;
its use is not assumed. No new hardware outcome is claimed before that run.

The VPK is compiled with VitaSDK. No additional automated tests were run
for this iteration; the physical run is needed to validate the startup path.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getacp
- https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getcpinfo
- https://learn.microsoft.com/en-us/windows/win32/api/winnls/ns-winnls-cpinfo
- https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT
