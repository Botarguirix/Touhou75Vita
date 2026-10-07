# Iteration 34: multimedia clock at game entry

Hardware iteration 33 returned GetModuleHandleA(NULL)=0x00400000 at
0x006424AA, then reached WINMM!timeBeginPeriod(1), return VA 0x00423A23.
It counted 111 serviced imports in 24,004,325 us, without a limit hit,
with a disarmed watchdog. The original disassembly shows the game function
0x00602A60 calls helper 0x004239F0 at 0x00602A77, returning to 0x00602A7C;
that helper makes the observed timer call. This is evidence that execution
has moved from CRT startup into the game's initialization path.

Iteration 34 serves timeBeginPeriod(1) and tracks outstanding requests.
timeEndPeriod(1) consumes one request; unmatched releases stop explicitly.
Other periods remain unsupported boundaries. The supported profile uses the
existing microsecond native process clock, requiring no resolution change.
timeGetTime returns its low 32-bit millisecond count, sharing the virtual
process-creation epoch of GetTickCount. Successful period requests return
TIMERR_NOERROR (zero). The common ABI check verifies stdcall cleanup and
preserved registers; LastError remains unchanged. This does not implement
multimedia callbacks or guarantee guest-thread scheduling accuracy. Only the
begin request has been observed on hardware; end/time APIs need coverage.

At the observed begin call, the probe reads the guest EBP chain and checks
the helper's saved caller is 0x00602A7C and the game frame's saved caller is
0x006424B0. Only both matches record startup_game_entry_verified=yes.
The results screen can then show GAME ENTRY CONFIRMED on a successful
diagnostic boundary. GAME BOOT: NOT YET VERIFIED remains because function
entry does not imply graphics, asset loading or a playable title screen.

Static code after the timer request also invokes synchronization and thread
creation imports. Those remain separate work; no guest worker thread is
fabricated or skipped in this iteration. The next stopped import will identify
the required contract. The 30-second watchdog remains unchanged.

Install 01.38 and confirm ITERATION 34 / GAME TIMER and
build_id=iteration34-timer-startup-r1. Logs: iteration34.log,
iteration34-runtime.log, iteration34-watchdog.log. Inspect the call-chain
marker, timer period request count, service ABI and next stopped import.
VitaSDK compilation is performed; no automated tests are added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod
- https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timeendperiod
- https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timegettime
