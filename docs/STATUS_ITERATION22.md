# Iteration 22: CRT environment conversion

Hardware iteration 21 reached WideCharToMultiByte at return VA 0x006493AF,
after creating a Unicode environment copy at 0x00C00510. The disassembly
shows a size query followed by allocation, conversion and environment release.

Iteration 22 serves the ASCII subset of WideCharToMultiByte with flags=0.
Pages ACP/OEMCP, 932, 1252 and UTF-8 are admitted only when every UTF-16 unit
is ASCII. Non-ASCII text, unsupported flags and other code pages remain explicit
diagnostic boundaries. This is not a complete Japanese text conversion layer.

A zero destination size queries required bytes. Positive input counts preserve
embedded NULs; -1 includes the first terminating NUL. A bounded 16384-unit
read protects terminated scans. Invalid parameters return 0/error 87, and a
short output returns 0/error 122 without writing. Invalid guest memory stops.
The output is read back and eight parameters plus return address consume 36
stack bytes. lpUsedDefaultChar is FALSE for successful ASCII conversion.

Tests cover empty environment query/write, explicit counts with embedded NULs,
terminated input, output guard bytes, stack canaries, insufficient buffers,
invalid pointers and non-ASCII diagnostic stops. Existing regressions remain.

Package 01.25. Logs: iteration22.log, iteration22-runtime.log and
iteration22-watchdog.log. Expected hardware evidence: size_query followed
by write, startup_conversion_readback=passed, then a later import boundary.
Full game boot is still unverified.

Contract: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
