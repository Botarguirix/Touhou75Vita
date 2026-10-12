# Iteration 42: 200 additional cases, 300 total

## Hardware evidence from iteration 41

The supplied logs report batch_passed=100, batch_failed=0. The original EXE
returned from SetCurrentDirectoryA and seven GetSystemMetrics calls. Its final
boundary was USER32.dll!LoadIconA with instance 0x00400000, return 0x00602B64.
startup_elapsed_us=27972451; the 30-second watchdog disarmed successfully.
The game title screen remains unverified.

## New batch coverage

Retains IDs 01–100 and adds exactly 200 cases. Every case logs its own ID,
parameters, name, result and error. These are parameterized service contracts
executed sequentially on Vita using synthetic API frames, after startup and its
watchdog. The batch restores main CPU/frame/LastError at the end. Each successful
API return checks stdcall cleanup and preserved registers in StartupServices.

| IDs | Count | Coverage |
| --- | --- | --- |
| 101–200 | 100 | Distinct ASCII/halfwidth CP932 strings, 1–4 repeated units plus explicit NUL; both conversion size queries, expected Unicode, reverse conversion, default flag and guards |
| 201–232 | 32 | Heap growth from sizes 1–32 to size+33, forcing relocation; patterned prefix, zeroed tail, exact size, LastError and cleanup |
| 233–264 | 32 | TLS walking bit and complement for every bit in a 32-bit value; initial zero, successful reads, freed-index error and cleanup |
| 265–284 | 20 | Explicit EXE module handle with capacities 0–19; XP truncation, exact NUL boundary, guard and LastError |
| 285–300 | 16 | Recursion depths 1–16, mixed Enter/TryEnter, plain/spin initialization, owner/lock/recursion state, LastError and deletion |

Temporary heap blocks and TLS slots are freed. Critical sections use separate
scratch addresses; acquired levels are released and sections deleted. Cleanup
failure fails the case. Synthetic passes provide contract coverage; graphics,
resources and the actual title screen require original game execution evidence.

## Original EXE scope and next priority

This iteration preserves the original startup behavior from iteration 41.
LoadIconA remains unsupported. The next implementation must locate and validate
the named RT_GROUP_ICON/RT_ICON resources in the original PE and track resource
objects before class registration and window creation. Missing or unsupported
resources must be recorded explicitly. Window creation, graphics initialization
and a presented original game frame are the milestones toward the title screen.

The diagnostic label now uses the actual log_path passed by main, so its
iteration number follows the log file automatically.

Install 01.46 / T075VITA1. Confirm ITERATION 42,
build_id=iteration42-batch-checks-r1 and batch_total=300. Collect iteration42.log,
iteration42-runtime.log and iteration42-watchdog.log. VitaSDK compilation
is checked locally; hardware outcomes of the new cases are pending.
