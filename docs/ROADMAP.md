# Touhou 7.5 Vita port roadmap

## Phase 0 — reproducible diagnostic shell (complete)

- Build and install a Vita application without proprietary game files in the repository.
- Verify the Japanese PE32 image, SHA-256, x86 register smoke tests, IAT bridge, heap, file, TEB/FS, TLS and process services.
- Record every run in a versioned log and show the active iteration on screen.

## Phase 1 — deterministic original entrypoint (checkpoint reached)

Goal: execute the original PE entrypoint far enough to observe a stable import boundary.

1. Serve `GetVersionExA`, `GetModuleHandleA` and `HeapCreate` with correct stdcall stack cleanup.
2. Serve `HeapAlloc`, `HeapFree` and `HeapSize` using a bounded guest heap.
3. Add an instruction trace and repeated-EIP guard so loops and bad returns are diagnosed instead of silently consuming budget.
4. Reach and record the first unsupported import after allocator initialization.

Hardware iteration 23 reached GetACP after HeapCreate, three HeapAlloc calls
and critical-section acquisition. Entry and import interception work, but
complete HeapFree/HeapSize behavior and instruction-level tracing remain work.

## Phase 2 — Win32 startup surface (current)

- Implement the small KERNEL32/USER32 contracts observed by the Japanese executable.
- Add file, timing, window, input and thread shims only when the log proves they are needed.
- Keep each service independently testable on Vita.

## Phase 3 — graphics and audio replacement

- Replace Direct3D 8 with a Vita renderer while preserving the game's resource and draw semantics.
- Replace DirectSound/MIDI paths with Vita audio services.
- Validate a title screen and one interactive scene before attempting full gameplay.

## Phase 4 — assets, input and save data

- Load the user's external Touhou data files from `ux0:data/TH075Vita`.
- Map keyboard commands to Vita controls.
- Add configuration, save paths and clean shutdown.

## Phase 5 — playable port and release validation

- Boot to menu, start a match, complete a round, and return to menu.
- Test on the remaining Vitas only after one device is stable.
- Package a reproducible VPK and document required original files and hashes.

### Current success criteria

Hardware iteration 24 returned from GetACP and GetCPInfo with the Japanese
CP932 profile and reached GetStringTypeW. Hardware iteration 25 passed that
probe and reached MultiByteToWideChar. Iteration 26 adds CP932 decoding and
classification of its character repertoire to continue CRT initialization.
Hardware iteration 26 decoded and classified a 256-character block and reached
LCMapStringW. Iteration 27 adds case mapping and reverse CP932 encoding.
Hardware iteration 27 completed those operations and released its critical
section. Iteration 28 serves GetModuleFileNameA for the original executable.
Hardware iteration 28 returned that path and reached the extensionless kernel32
query. Iteration 29 resolves that alias and the CRT's processor erratum query.
Hardware iteration 29 passed both and reached SetUnhandledExceptionFilter.
Iteration 30 implements filter registration state; exception dispatch remains
an independent missing subsystem.
Hardware iteration 30 R2 registered the filter and reached HeapReAlloc.
Iteration 31 tracks allocations, corrects HeapSize/HeapFree and preserves
contents on resizing. Free-space recycling remains needed for sustained play.
Hardware iteration 31 returned HeapSize=128 and reached GetSystemTimeAsFileTime
without a limit hit. Its changed path did not exercise HeapReAlloc.
Iteration 32 adds UTC time, guest process identity and monotonic clocks for
the CRT initialization routine, with output readback and explicit clock sources.
Hardware iteration 32 passed the observed clocks and reached the repeated
GetModuleHandleA(NULL) query just before the call to 0x00602A60. Iteration 33
corrects that query contract; reaching its return site alone is not proof that
the game function executed or that the game booted.
Hardware iteration 33 reached timeBeginPeriod(1) in the helper called from the
game's main function, providing evidence of game initialization execution.
Iteration 34 serves that timer request and checks its saved caller chain.
Hardware iteration 34 verified the saved game-entry caller chain, returned the
timer request and reached CreateEventA. Iteration 35 tracks unnamed events;
blocking waits and actual guest workers remain scheduler work.
Hardware iteration 35 created the event and reached CreateThread. Iteration 36
diagnoses the original worker's first import with its own stack/TEB while the
main remains stopped. Full scheduling and thread creation remain required.
Hardware iteration 36 executed the worker to its first WaitForSingleObject.
Iteration 37 saves that wait context and resumes main after the observed
CreateThread. Continuous scheduling, event/time wakeups and priority handling
are still required; this is only the initial context handoff.
Hardware iteration 37 passed the initial handoff and reached SetThreadPriority.
Iteration 38 batches four independent event/heap/clock service checks to improve
hardware coverage per install; it retains that original startup boundary.
Hardware iteration 38 passed all four service checks. Iteration 39 expands
coverage to ten checks, including closed handles, failure preservation, TLS,
Japanese conversion, LastError and pathname buffer boundaries. These checks
retain the original SetThreadPriority startup boundary.
Hardware iteration 39 passed ten service checks. Iteration 40 adds ten more,
serves worker priority state and dispatches one real timed wake, recording the
next worker/main boundary. Continuous scheduling and rendering remain pending.
The next major milestone is returning from game initialization's synchronization
and thread setup. Then record actual game asset requests and
the first graphics calls before implementing a renderer for a real game frame.
No reliable iteration count or completion percentage is available yet.

Hardware iteration 40 passed twenty cases, resumed the original worker after
its real timeout and reached SetCurrentDirectoryA on main. Iteration 41 mounts
the application directory and serves the observed logical window dimensions,
with 100 individual service cases per installation. Next priorities are the
actual resource/window boundary, guest window creation, then a real game frame
through the graphics backend. The diagnostic screen is not a game title screen.
Hardware iteration 41 passed 100 service cases and reached LoadIconA after the
original directory setup and seven window metric queries. Iteration 42 adds
200 cases (300 total) while preserving this original EXE boundary. The next
implementation priority is real PE icon/resource loading, followed by window
registration/creation and the graphics backend for an original game frame.
