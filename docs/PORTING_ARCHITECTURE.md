# Porting architecture review

**Date:** 2026-09-30  
**Status:** Architecture proposal; the repository is an analysis prototype, and gameplay boot has not been implemented.

## Executive decision

Keep the supplied PE, disassembly, and analysis pipeline as project inputs. Before investing further in the hand-written x86 semantic runtime, evaluate a D2Vita-style execution path: load the original PE32 executable and execute its x86 code through an ARMv7 dynamic recompiler, while replacing Windows APIs with Vita-side bridges. Keep the current static recompilation for offline analysis and hot-function identification. Its runtime needs much broader semantic coverage before it can execute the game.

This is a decision to run a compatibility proof of concept, not a claim that D2Vita's engine can be reused unchanged. Check engine availability, license, PE loader behavior, CPU coverage, and the imports required by Touhou 7.5 before selecting it.

## What the current prototype actually does

The last VPK confirmed by the user's hardware log was the Iteration 01 diagnostic: it opened `ux0:data/TH075Vita/TH075.exe`, checked its DOS/PE headers, and wrote `iteration01.log`. The local worktree now contains an Iteration 06 diagnostic that separately probes `TH075.exe` and `TH075E.exe` and writes `iteration06.log`, but that revision has not yet been built or confirmed on hardware. Neither diagnostic maps sections, resolves imports, executes x86 instructions, or starts the game.

The supplied reverse-engineering ZIP already contains an import listing. It shows 157 static imports across DINPUT8, WINMM, d3d8, KERNEL32, USER32, GDI32, ADVAPI32, and ole32. It also imports `LoadLibraryA` and `GetProcAddress`, so the static table may not cover every runtime dependency. The PE has relocations stripped and no base-relocation directory. A source review of WinVita's `Bridge::add_module()` found an explicit path for `IMAGE_FILE_RELOCS_STRIPPED`: it keeps the module at its preferred base. That matches Touhou's `ImageBase=0x00400000` and removes one loader concern, but we still need to prove that WinVita maps this exact image and its sections successfully; this finding alone does not prove imports resolve or the game can run.

The Japanese executable's identity is established for the supplied analysis package: the hash computed from its embedded `TH075.exe` is `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98`, matching the hash the user supplied for the Japanese build. The package's `pe_fingerprint.json` instead reports `a3aca32a61201a77a8dccd743725ee08ca5db311a3eec7d05b36b7a1e5dd7cf8` and subsystem 0, while the import report and Vita log say subsystem 2; treat that JSON fingerprint as stale or from a different executable. The Japanese hardware log matches the file's size and visible PE fields, but it does not include a hash, so exact byte identity of the copy installed on Vita is not cryptographically confirmed. The user also supplied the hash `C8313228A98134B5CEB75027D77B302E4271670B17F6B52593A1D41679834D0E` for `TH075E.exe`, which they identify as a small patch needed to run the English translation with the Japanese game. Its Vita probe reports 9,728 bytes, six sections, and a PE32/I386 entry point at `0x00401240`. The patch has not been executed or analyzed for imports/behavior; it is not the main game image to load.

The generated C is an instruction-level translation scaffold; it does not recover the original source structure. The semantic runtime currently has correctness gaps that can change game behavior:

- Most integer helpers read and write operands as 32-bit values. MOVZX and MOVSX currently call the same behavior as MOV.
- Guest memory is stored as uint32_t words. Byte, word, and unaligned accesses use mem[address / 4] without selecting the byte offset; narrow writes overwrite the whole word.
- Arithmetic flags are incomplete. The generic arithmetic helper does not calculate the x86 overflow, parity, auxiliary-carry, or carry rules correctly; several signed and parity conditions in iamp_cond always return false.
- The x87 and MMX/SSE implementations cover only subsets. Unknown SIMD operations can preserve the destination and continue, which hides incorrect execution rather than reporting it.
- recover_functions.py guesses starts from call targets and common prologues, makes function extents from the next candidate, and trims at the first ret. That can split valid control flow and misclassify linear-disassembly padding as code.
- A conditional branch emitted to a different function changes EIP without returning to a dispatcher. The Vita entry point also has no loop that resumes execution at the new EIP.

These issues mean the current output cannot be treated as evidence that the game itself has been translated or can boot.

## What D2Vita contributes

D2Vita runs the original Diablo II 1.14d PE32 under a Win32/x86 runtime: a PE loader, import bridge, and x86-to-ARMv7 dynamic recompiler derived from Box86. It replaces the system layer with ARM/VitaSDK implementations and has a desktop ARM reproduction path in addition to Vita builds. Its roadmap measures changes on hardware and tracks memory, performance, and crash evidence instead of relying on a successful compile alone.

That is the closest architectural precedent for executing the original x86 game on Vita. The runtime translates x86 blocks while loading PE and bridging Windows APIs. Its Diablo-specific graphics path handles Glide3x and GXM; Touhou 7.5 imports Direct3D 8, which needs a separate backend.

The D2Vita repository's `.gitmodules` points `third_party/winx86` to the separate [`Franckrst/WinVita`](https://github.com/Franckrst/WinVita) repository. WinVita describes itself as a generic PE32 loader plus an ARMv7 x86 dynarec derived from Box86, a CPU interface, guest-thread schedulers, and a Win32 import bridge. Its bridge resolves imports from already loaded guest modules before consulting native shims. It also has the stripped-relocations/preferred-base path noted above. It provides no prewritten Win32 shims and no game-specific hooks. So D2Vita does **not** port or emulate Windows XP as a complete operating system: it runs the original Diablo II executable and supplies the selected Win32 behavior that game needs. That distinction matters for Touhou too: we need the executable's actual API imports and behavior, then our own game-specific boot and graphics/audio/input bridges.

WinVita's own code is MIT-licensed, while its bundled components retain their separate notices and licenses. If we consider it as a submodule, pin a revision and preserve its license files and third-party notices; the D2Vita repository's GPL-3.0 license does not by itself describe every component in its dependency tree. This is an architecture and licensing lead, not yet a decision to vendor it.

Apply the same boundary to the import bridge:

- Inventory imports from the exact executable supplied for analysis, then classify each implementation by its behavior and caller context. A Win32 API name alone does not tell us whether a shim is generic; a seemingly generic function may contain a game-specific path or policy.
- Separate imports implemented by another guest PE DLL from Windows system imports that need a native Vita bridge. The generic loader can resolve guest modules; only unresolved OS behavior belongs in the shim inventory.
- Keep the Win32-to-Vita mechanism generic and put Touhou-specific behavior in one game layer. Register each import key in one place because duplicate registrations can silently replace earlier handlers.
- Do not move D2-specific hooks into a generic engine. D2's Glide-to-GXM renderer, DCC decoder, audio choices, and cell engine solve Diablo-specific problems. Touhou's Direct3D 8 path needs its own rendering investigation and GXM implementation.

D2Vita also separates validation by what each environment can prove: an ARM correctness/replay level, the same Vita binary under Vita3K, and physical-console measurements. For this project, CI proves that the VPK builds; Vita3K can check that the same VPK starts and writes its report; the user's Vita confirms console file access and behavior. Performance claims must wait for repeatable measurements on the physical console. A successful PE probe proves none of the later levels.

Repentogxm is the other useful precedent: it statically translates an unpacked PC executable to C and builds that output for Vita. It validates the idea of an ahead-of-time translator, but the Touhou prototype needs substantially more complete x86 semantics, control-flow recovery, PE loading, and API bridges before that route is viable.

References: [D2Vita README](https://github.com/Franckrst/D2Vita), [D2Vita ARCHITECTURE.md](https://github.com/Franckrst/D2Vita/blob/main/ARCHITECTURE.md), [D2Vita architecture notes](https://franckrst.github.io/D2Vita/architecture/), [WinVita README](https://github.com/Franckrst/WinVita/blob/main/README.md), [WinVita license](https://github.com/Franckrst/WinVita/blob/main/LICENSE), [WinVita third-party notices](https://github.com/Franckrst/WinVita/blob/main/THIRD-PARTY-NOTICES.md), [D2Vita roadmap](https://github.com/Franckrst/D2Vita/blob/main/ROADMAP.md), and [repentogxm](https://github.com/0xl0cal/repentogxm).

## Proposed project structure

Separate the generic x86 execution engine from Touhou-specific platform code, following D2Vita's boundary:

- analysis/ and tools/: PE fingerprinting, disassembly, function/control-flow analysis, import inventory, and data-file inspection. Keep these reproducible and independent of the Vita executable.
- runtime/x86/: selected generic loader/CPU engine, if the license and compatibility audit permit reuse. Keep game-specific changes out of this layer.
- runtime/win32/: import bridge and Windows API shims. Log every unresolved import and fail clearly rather than returning silent success.
- vita/: Vita startup, controls, filesystem mapping, audio, and rendering backends.
- docs/: the supported PE version, build steps, validation results, known gaps, and hardware measurements.

Keep proprietary executable and game data outside Git. Record hashes and let users supply their own files.

## Proof-of-concept gates

1. **Reproducible input:** reconcile the reverse-engineering ZIP's fingerprint mismatch and verify the hash of the exact executable used on Vita. Pin the exact build before interpreting addresses. Keep the executable out of Git.
2. **Engine audit:** the initial source and license review found WinVita's PE32 loader, ARMv7 dynarec, import bridge, and explicit support for stripped-relocation images at their preferred base. Before choosing it, pin a revision and verify its build procedure, instruction coverage, Vita memory/address-space assumptions, and third-party notices. Do not copy D2-specific code or assume its license covers the generic engine.
3. **CPU oracle:** validate the supported instruction subset against the original x86 behavior on a desktop harness before Vita integration. Include x87, MMX/SSE, flags, byte/word memory operations, indirect branches, and exceptions found in Touhou's reachable code.
4. **Loader and imports:** first make a loader-only proof with WinVita: map headers and sections at `0x00400000`, inspect import resolution, and log unresolved modules/symbols without calling the PE entry point. Then inventory dynamic LoadLibraryA/GetProcAddress use, classify imports by observed behavior and caller context, and add one explicitly owned bridge per exercised import. Report unresolved calls instead of returning silent success.
5. **First Vita vertical slice:** reach a controlled diagnostic through the Vita loader, then bring up input and a minimal visible frame. Treat Direct3D 8 translation to sceGxm or a compatible layer as a separate workstream; D2Vita's Glide ring is not a Direct3D 8 backend.
6. **Game services:** add the needed DirectInput, WinMM/audio, file, timer, window/message, and COM behavior based on observed calls. Keep generic Win32 mechanisms separate from Touhou-specific policy, and keep unsupported APIs visible in logs.
7. **Validation evidence:** label each result by level: host/CI correctness and deterministic checks, Vita3K behavior with the same VPK, or physical-console behavior. Record boot result and crash state at each level. Record performance and graphics-fidelity claims only from real hardware, with repeatable controls and image captures where relevant.

## Immediate next iteration

The next engineering milestone is to build a loader-only WinVita proof for the Japanese game image, mapping its headers and sections at the preferred base and reporting import-resolution results. Do not execute the entry point in this milestone. Afterward, inspect how `TH075E.exe` applies the English patch: identify its file dependencies, imported APIs, and whether it modifies the Japanese executable or its data. The 9,728-byte patch is a separate optional translation layer, not the main game image. The Japanese static import list is already available; separately inspect dynamic LoadLibraryA/GetProcAddress behavior and classify guest DLLs versus Win32 APIs. Preserve the current translator as an analysis tool. If WinVita passes the loader and license/build audit, continue to controlled CPU execution; if it fails, return to static recompilation with a real decoder/IR, verified control-flow boundaries, width-correct guest memory, complete flag semantics, and differential CPU checks.
