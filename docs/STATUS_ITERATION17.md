# Iteration 17: nested SEH and named KERNEL32 module

Iteration 16 returned from HeapCreate with 16-byte stack cleanup and reached
GetModuleHandleA("kernel32.dll") at return address 0x00650568. Its SEH head
was 0x009FEEB8, pointing to the earlier record at 0x009FEFE8. The old
single-record check wrongly rejected that legitimate nested chain.

Iteration 17 walks at most 32 stack records, checks readable aligned frames,
rejects cycles and verifies handlers lie inside the loaded image. The exact
original entrypoint record remains required at the first import. This
validates registration only; exception dispatch is still unimplemented.

Named KERNEL32 queries return an opaque runtime module handle (0x00AB1000).
This identifies the compatibility services; it does not load a Windows DLL.
Other named modules remain unsupported. Static code shows the next call is
GetProcAddress for InitializeCriticalSectionAndSpinCount. That API remains
a diagnostic stop in this iteration.

Success means startup_seh_chain_depth=2, the named module query serviced,
and a later unsupported import reached. It does not mean full game boot.

Install version 01.20 on the same Vita. Logs: iteration17.log,
iteration17-runtime.log and iteration17-watchdog.log.
