# Iteration 18: resolve and call a dynamic startup export

Iteration 17 passed on hardware: nested SEH was accepted, HeapCreate returned,
GetModuleHandleA("kernel32.dll") was served and GetProcAddress was reached at
return VA 0x00650578. Full boot remains unverified.

Iteration 18 reads the requested export name from guest memory and resolves
only InitializeCriticalSectionAndSpinCount for the runtime KERNEL32 handle.
It returns the reserved trap VA 0x00BFFFE0, separate from all IAT entries and
the exit sentinel. When the EXE calls that function pointer, the trap dispatch
applies the usual SEH/frame checks and invokes its initializer.

The initializer writes and reads back a 24-byte x86 critical-section state.
Both GetProcAddress and this initializer pop 12 bytes (return address plus
two arguments). Duplicate initialization and unreadable buffers are rejected.
Enter/Leave/Delete operations and multithreaded synchronization are not yet
implemented; an unsupported call remains a diagnostic stop, not a success stub.

Expected evidence: startup_export_requested, startup_export_resolved_va,
startup_dynamic_export_called=yes, startup_critical_section_readback=passed,
startup_critical_init_calls greater than zero, then a later import boundary.

Package version 01.21. Files in ux0:data/TH075Vita: iteration18.log,
iteration18-runtime.log and iteration18-watchdog.log. Use the same physical Vita.

Reference contract: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount
