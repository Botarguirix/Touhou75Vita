# TH075 Function Recovery — Iteration 02

This iteration replaces the previous "linear region" inventory with a call/prologue driven function recovery pass and a basic-block CFG pass.

## Results

- PE entry point: `0x64232c`
- `.text`: `0x401000 .. 0x657000`
- Instructions in `.text`: **699,357**
- Candidate function starts: **3,860**
- Starts supported by a classic function prologue: **3,198**
- Direct CALL targets: **2,984**
- Direct branch targets: **37,042**
- Direct control-flow edges: **107,550**
- High-confidence starts (entry/call-target/prologue): **2,324**
- Basic blocks were recovered for every candidate function.

## Important correction

The previous project reported 402,239 "functions/regions". Those were mostly discontinuities in the linear disassembly and were **not reliable function boundaries**. They are no longer used as the function model.

The new model uses:

1. PE entry point.
2. Direct `CALL` destinations inside `.text`.
3. Classic `push ebp; mov ebp, esp` prologues.
4. Hot-patch style `mov edi, edi; push ebp; mov ebp, esp` prologues.
5. Direct conditional/unconditional branch destinations to split basic blocks.

This is still conservative: indirect calls/jumps and compiler-generated tail calls require a later data-flow pass.

## Generated artifacts

- `functions_recovered.json` — recovered function candidates and CFG blocks.
- `cfg_blocks.json` — basic-block map.
- `callgraph.json` — direct CALL/branch edges.
- `recovery_report.json` — machine-readable statistics.
- `th075.ir.jsonl` — typed instruction IR stream for the `.text` section.
- `th075.ir.summary.json` — IR coverage summary.

## IR coverage

The instruction stream is now classified into semantic categories rather than being emitted as untranslated stubs:

- data/memory/register operations
- integer arithmetic
- control flow
- flags/set/cmov
- x87
- SIMD/MMX/3DNow
- miscellaneous
- explicitly unsupported instructions

The current IR pass recognizes **699,357** instructions and flags **5,601** occurrences as unsupported/unknown categories. These are not silently discarded; their mnemonics are listed in `th075.ir.summary.json`.

## Next implementation stage

The next pass is the semantic lifter/code generator. It must:

- convert operands into typed IR expressions;
- model EFLAGS precisely;
- model x87 stack state;
- model MMX aliasing and SSE registers;
- recover indirect control flow;
- bind imported Win32/D3D8 functions;
- generate C from basic blocks rather than entry stubs.

No VPK is claimed by this iteration. It is a materially stronger reverse-engineering/recompilation base, but the native Vita runtime and complete instruction semantics are still required before a playable VPK can be produced.
