# IAMP Static Recompilation Status — Iteration 02

## Completed

- Preserved the original PC executable as a reference input.
- Recovered the PE `.text` range and entry point.
- Replaced the old linear-region inventory with call/prologue-driven function candidates.
- Recovered direct control-flow edges and basic blocks.
- Added a typed instruction IR stream covering the complete `.text` instruction inventory.
- Explicitly reports unsupported/unknown mnemonics instead of silently emitting stubs.

## Current measurements

- `.text`: `0x401000 .. 0x657000`
- Instructions: 699,357
- Function candidates: 3,860
- Prologue starts: 3,198
- Direct CALL targets: 2,984
- Direct branch targets: 37,042
- Direct edges: 107,550
- IR unsupported/unknown occurrences: 5,601

## Not yet complete

- Complete x86 semantic lifter/code generator
- Indirect branch/call recovery
- x87/MMX/SSE semantic runtime
- Win32 import bridge
- D3D8 compatibility layer
- Vita input/audio/filesystem backends
- VitaSDK compilation in this environment
- Final SELF/VPK generation

The project is therefore not yet a playable VPK.
