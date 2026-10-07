# Iteration 27: Japanese case mapping and reverse encoding

Hardware iteration 26 queried and wrote 256 decoded UTF-16 units from CP932,
then classified the 256 units successfully. Both output readbacks and ABI
checks passed. The run recorded 50 counted startup calls, four conversion
calls and two classification calls. The next boundary was LCMapStringW,
return VA 0x0065278A; the watchdog was disarmed and no execution limit hit.

Disassembly shows LCMapStringW(0, LCMAP_LOWERCASE, 0x006639D0, 1, NULL, 0):
a size query on NUL to probe Unicode API availability. Subsequent CRT code
maps its character table and converts the mapped Unicode back to CP932.

Iteration 27 supports LCMAP_LOWERCASE (0x100) and LCMAP_UPPERCASE (0x200).
Locale 0 (CRT probe), 0x411 (Japanese) and system/user default aliases select
the explicit Japanese profile. The generator captures native Windows Japanese
case mappings for the existing CP932 Unicode repertoire; this is host Windows
data, not an exact Windows XP locale database. One-unit mappings only are
admitted. Expansions, characters outside the repertoire, other locales,
sort keys and additional mapping flags remain diagnostic boundaries.

Positive counts preserve embedded NUL; negative counts include first NUL.
Size queries do not touch output. Same-buffer case mapping is supported by
staging input; partial overlap is rejected. Short capacity returns error 122,
invalid parameters return error 87. Input is bounded to 16384 units, output
is read back, and stdcall cleanup is 28 bytes with preserved-register checks.

WideCharToMultiByte now uses strict canonical CP932 encoding for pages 932,
0 and 1, including two-byte Japanese and vendor characters. Encoding mappings
come from .NET exception fallback through the reproducible PowerShell generator.
Unmappable characters remain explicit boundaries; best-fit/replacement behavior
is not implemented. Successful mappings report usedDefaultChar=FALSE. Other
previously admitted pages retain their ASCII subset. Generated tables contain
no game data and require no Windows library on Vita.

Install 01.30 and confirm ITERATION 27 / iteration27-case-map-startup-r1.
Logs: iteration27.log, iteration27-runtime.log, iteration27-watchdog.log.
Expected evidence: serviced LCMapStringW, startup_case_error=0, readback for
write calls, and a later startup boundary. Game boot is still unverified.
VitaSDK compilation is performed; no automated tests were added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-lcmapstringw
- https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
- https://learn.microsoft.com/en-us/dotnet/api/system.text.encoding.getencoding
