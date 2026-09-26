# IaMP Vita Static Recompiler — Iteration 03

## What changed

This iteration moves from instruction classification to a **semantic C translation layer** for the recovered function set.

### Implemented
- Recovered-function set: 3,860 candidates.
- Generated semantic C for 253,689 instructions across the recovered functions.
- Direct calls are dispatched through a generated `iamp_dispatch()` table.
- Direct calls preserve a synthetic x86 return address on the emulated stack.
- `ret` restores the translated return address.
- Generic x86 register state and flags runtime.
- Basic x86 memory operands, including base/index/scale/displacement expressions.
- Segment-prefixed absolute operands such as `ds:0x671210`.
- Integer/data instruction helpers for the common x86 subset.
- Conditional branch helpers for the implemented flag subset.
- x87 stack emulation for the high-frequency operations found in IaMP:
  - `fld`, `fst`, `fstp`, `fild`, `fistp`
  - `fld1`, `fldz`, `fldpi`
  - `fadd`, `fsub`, `fmul`, `fdiv`, reverse variants
  - `faddp`, `fsubp`, `fmulp`, `fdivp`, reverse variants
  - `fcom`, `fcomp`, `fucompp`
  - `fxch`, `fchs`, `fabs`
  - `fnstsw`/`fstsw`, `fninit`
  - `sahf` bridge for x87 compare/status flows
- Host-side compile validation succeeds for the generated C and runtime.
- Smoke test successfully executes the first recovered function, including stack-frame manipulation and an absolute data-memory increment.

## Deferred work

The largest remaining instruction families are MMX/3DNow, SSE/SSE2, string/REP instructions, and a smaller collection of system/legacy instructions.

The generated C deliberately traps on deferred instructions rather than silently returning incorrect values.

## Important limitation

This is still **not a playable Vita build**. The semantic layer now executes a real subset of the original x86 semantics, but the following are still required before a final VPK can honestly be produced:

1. Complete remaining instruction semantics used by the game.
2. Resolve all indirect calls/imports.
3. Implement Win32 API shims used by the executable.
4. Implement D3D8 functionality through a Vita graphics backend.
5. Implement DirectInput/audio/timer/file-system replacements.
6. Integrate the original external game data (`th075.dat` and `th075bgm.dat`).
7. Build with VitaSDK and package SELF/VPK.
8. Test on actual Vita hardware and fix runtime-specific faults.

## Validation

The generated semantic translation and runtime pass GCC syntax/compile validation on the host. The smoke test reports:

- EAX preserved from ECX input: `0x12345678`
- ESP restored: `0x00700000`
- Return EIP restored from the synthetic stack: `0xDEADBEEF`
- Absolute data location `0x00671210` incremented from `0` to `1`.

This validates the basic register/memory/stack/return mechanism, not the whole game.
