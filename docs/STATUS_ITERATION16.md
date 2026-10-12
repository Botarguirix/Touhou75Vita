# Iteration 16: correct stdcall return frames

Iteration 15 reached HeapCreate then exhausted its budget at EIP 0x00003FE0.
Its final ESP was 0x009FEEE0. The backend's trap_epilogue adds its esp_add
argument directly; it does not separately pop the return address.

HeapCreate, HeapAlloc, HeapFree and HeapSize take three 32-bit arguments.
Their full return frame is 16 bytes: 4 for the return address and 12 for
arguments. Iteration 15 advanced ESP by only 12 bytes. Iteration 16 fixes
all four services and derives the ABI expectation from parameter count.

The log records startup_service_stack_bytes=16 for heap calls. Hardware
validation must show progression beyond HeapCreate; reaching a later
unsupported import is a startup checkpoint, not proof of a full game boot.

Install the Iteration 16 VPK on the same Vita. Logs are iteration16.log,
iteration16-runtime.log and iteration16-watchdog.log in ux0:data/TH075Vita.
Screen and package version identify iteration 16 (01.19).
