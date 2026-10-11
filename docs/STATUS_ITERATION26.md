# Iteration 26: CP932 decoding and character repertoire

Hardware iteration 25 serviced GetStringTypeW's NUL probe at 0x0064DA32,
with output and stdcall ABI checks passing. It recorded 47 counted startup
calls and stopped at MultiByteToWideChar(932, ...) returning to 0x0064DAA6.
The watchdog was disarmed and the execution limit was not reached.

Iteration 26 adds MultiByteToWideChar for CP932 and aliases 0, 1 and 3
under the Japanese profile. The compiled mapping includes single-byte,
halfwidth kana, double-byte Japanese and vendor/private-use mappings.
The immutable decoder tables were generated using .NET's CP932 codec with
exception fallback. The included PowerShell generator reads no game data.

Supported flags: 0, MB_PRECOMPOSED (1), MB_ERR_INVALID_CHARS (8), or 9.
Composite/glyph modes and other code pages remain explicit boundaries.
Malformed input with flag 8 returns zero and ERROR_NO_UNICODE_TRANSLATION;
without it execution stops, because exact XP drop/recovery semantics remain
pending. Invalid parameters return error 87; short output returns error 122.
Input is bounded to 16384 bytes. -1 includes terminating NUL; positive counts
preserve embedded NULs. Capacity zero queries the size without touching output.
Conversions are prepared before writing, checked for overlap and read back.
Stack cleanup is 28 bytes and preserved registers are checked.

GetStringTypeW CT_CTYPE1 now supports the decoder's 9402 distinct Unicode
units, using native Windows classification captured by the generator. This
is a fixed table, not a Vita dependency on Windows. Its source is the host's
Windows classification, not an exact Windows XP locale database. Characters
outside the repertoire and CT_CTYPE2/3 stop explicitly. The existing reverse
WideCharToMultiByte direction still supports only its ASCII subset.

Install 01.29; confirm ITERATION 26 and build_id=iteration26-cp932-startup-r1.
Logs: iteration26.log, iteration26-runtime.log, iteration26-watchdog.log.
Look for startup_decode_mode=size_query/write, startup_decode_error=0,
startup_decode_readback=passed when writing, and the next stop import.
Successful game boot is not claimed before hardware evidence.

VitaSDK compilation is performed; no automated tests were added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
- https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-getstringtypew
- https://learn.microsoft.com/en-us/dotnet/api/system.text.encoding.getencoding
