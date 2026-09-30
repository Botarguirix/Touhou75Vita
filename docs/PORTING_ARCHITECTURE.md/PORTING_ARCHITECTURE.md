# Porting architecture review

**Date:** 2026-09-29  
**Status:** Architecture proposal; the repository is an analysis prototype, and gameplay boot has not been implemented.

## Executive decision

Keep the supplied PE, disassembly, and analysis pipeline as project inputs. Before investing further in the hand-written x86 semantic runtime, evaluate a D2Vita-style execution path: load the original PE32 executable and execute its x86 code through an ARMv7 dynamic recompiler, while replacing Windows APIs with Vita-side bridges. Keep the current static recompilation for offline analysis and hot-function identification. Its runtime needs much broader semantic coverage before it can execute the game.

This is a decision to run a compatibility proof of concept, not a claim that D2Vita's engine can be reused unchanged. Check engine availability, license, PE loader behavior, CPU coverage, and the imports required by Touhou 7.5 before selecting it.

## What the current prototype actually does

The checked-in vita/src/main.c allocates 16 MiB of zero-filled memory, places a hard-coded entry address (0x64232c) in EIP, and dispatches once. It does not map PE sections, apply relocations, create a Windows process environment, or resolve imports. The existing file is an initial CPU-dispatch stub; PE loading and game startup remain unimplemented.

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

Repentogxm is the other useful precedent: it statically translates an unpacked PC executable to C and builds that output for Vita. It validates the idea of an ahead-of-time translator, but the Touhou prototype needs substantially more complete x86 semantics, control-flow recovery, PE loading, and API bridges before that route is viable.

References: [D2Vita README](https://github.com/Franckrst/D2Vita), [D2Vita architecture](https://github.com/Franckrst/D2Vita/blob/main/ARCHITECTURE.md), [D2Vita roadmap](https://github.com/Franckrst/D2Vita/blob/main/ROADMAP.md), and [repentogxm](https://github.com/0xl0cal/repentogxm).

## Proposed project structure

Separate the generic x86 execution engine from Touhou-specific platform code, following D2Vita's boundary:

- analysis/ and tools/: PE fingerprinting, disassembly, function/control-flow analysis, import inventory, and data-file inspection. Keep these reproducible and independent of the Vita executable.
- runtime/x86/: selected generic loader/CPU engine, if the license and compatibility audit permit reuse. Keep game-specific changes out of this layer.
- runtime/win32/: import bridge and Windows API shims. Log every unresolved import and fail clearly rather than returning silent success.
- vita/: Vita startup, controls, filesystem mapping, audio, and rendering backends.
- docs/: the supported PE version, build steps, validation results, known gaps, and hardware measurements.

Keep proprietary executable and game data outside Git. Record hashes and let users supply their own files.

## Proof-of-concept gates

1. **Reproducible input:** recompute the executable SHA-256 and PE metadata from th075.exe; reconcile the stale fingerprint fields noted in the reverse-engineering package. Pin the exact executable build before interpreting addresses.
2. **Engine audit:** inspect the D2Vita winx86 submodule's source availability, license, build procedure, and PE32/x86 instruction coverage. Decide whether to reuse it as a separate dependency, adapt another compatible engine, or continue the static translator. Do not copy D2-specific code or assume its license covers the generic engine.
3. **CPU oracle:** validate the supported instruction subset against the original x86 behavior on a desktop harness before Vita integration. Include x87, MMX/SSE, flags, byte/word memory operations, indirect branches, and exceptions found in Touhou's reachable code.
4. **Loader and imports:** map PE sections and relocations, establish a guest stack/process environment, resolve imports, and inventory dynamic LoadLibraryA/GetProcAddress use. Add explicit bridges for the imports that are actually exercised.
5. **First Vita vertical slice:** reach a controlled diagnostic through the Vita loader, then bring up input and a minimal visible frame. Treat Direct3D 8 translation to sceGxm or a compatible layer as a separate workstream; D2Vita's Glide ring is not a Direct3D 8 backend.
6. **Game services:** add the needed DirectInput, WinMM/audio, file, timer, window/message, and COM behavior based on observed calls. Keep unsupported APIs visible in logs.
7. **Hardware evidence:** record boot result, frame time, memory/JIT usage, and crash state on Vita3K and real hardware. Every roadmap item should say which environment was used and what was actually observed.

## Immediate next iteration

The next engineering milestone should establish engine compatibility and generate a reproducible PE/import report before expanding generated C output. Preserve the current translator while doing that comparison. If the runtime route passes its audit, make a small loader-plus-import-bridge prototype and reuse the recovered disassembly to prioritize only the APIs and instructions it reaches. If it fails, return to static recompilation with a real decoder/IR, verified control-flow boundaries, width-correct guest memory, complete flag semantics, and differential CPU checks.
