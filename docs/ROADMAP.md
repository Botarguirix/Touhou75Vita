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

Iteration 24 should return from GetACP with the Japanese CP932 profile and
record the next boundary. The next major milestone is completing CRT startup
and reaching game initialization. Then record actual game asset requests and
the first graphics calls before implementing a renderer for a real game frame.
No reliable iteration count or completion percentage is available yet.
