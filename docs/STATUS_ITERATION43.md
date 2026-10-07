# Iteration 43: real PE resource loading and window class registration

Hardware iteration 42 passed 300/300 checks and reached LoadIconA with main
startup elapsed 27436732 us and the 30-second watchdog disarmed.

## Implemented original-game services

- LoadIconA: bounds-checked PE32 resource tree traversal, named or ordinal
  RT_GROUP_ICON and referenced RT_ICON lookup, deterministic neutral/Japanese
  language preference and size/header checks. The observed 32x32 8-bit indexed
  DIB is decoded from its palette, bottom-up XOR bitmap and AND transparency
  mask into 1024 ABGR pixels. Other image encodings stop explicitly.
- The actual EXE contains ICON1/ICON2/ICON3, each referencing its own 2216-byte
  DIB, language 1041. Both observed startup calls request ICON1. Shared handles
  are cached against owned decoded objects. Missing resources return NULL and
  error 1814; no substitute game icon is fabricated.
- LoadCursorA(NULL, IDC_ARROW): a tracked 32x32 arrow object with hotspot 0,0.
  Only that predefined cursor is supported; cursor drawing/input integration
  remains pending.
- GDI32 GetStockObject(WHITE_BRUSH): a tracked solid white brush identity.
- RegisterClassExA: validates the 48-byte guest layout, executable procedure,
  instance and tracked icons/cursor/brush, records the full class structure,
  returns a unique atom, rejects duplicate registration with error 1410.
- Icon/cursor dimensions 11–14 return 32 in the logical desktop profile.

All returned services use the existing stdcall/state checks. The diagnostic
screen previews the actual decoded EXE icon at 2x scale. This is resource-load
evidence, not the game's title screen. Hardware validation is pending.

CreateWindowExA remains a controlled boundary. It records requested width,
height and styles. Window creation must execute synchronous guest
WM_NCCREATE/WM_NCCALCSIZE/WM_CREATE callbacks and maintain native presentation
state before returning a successful HWND. That callback dispatcher, a D3D8
backend, continuous scheduling and interactive game input remain work.

## Host evidence collected during this iteration

The read-only inventory tool implements the archive table described by
arc_conv's arc_th075.asm, validates every file extent and decodes CP932 names.
It found 215 entries in th075.dat, 37 in th075b.dat and 34 WAV entries in
th075bgm.dat. Named system entries include battle, select, load, logo and title.
The title container begins with a 640x480 descriptor. Internal image/container
decoding still requires analysis; the inventory alone does not render assets.
Local artifacts include archive-inventory.json; original assets stay out of Git.

The official apitrace x86 Windows build (git-706f9d61) captured the original EXE
from an isolated local copy. The first trace records 776 Present frames,
173 texture creations and 15739 DrawPrimitiveUP calls. A final frame was
replayed successfully: the introduction's mountains/dialogue, not the menu.
Observed device setup is windowed 640x480 X8R8G8B8 with D16 depth; textures use
A8R8G8B8, point filtering, source-alpha blending and XYZRHW/diffuse/UV vertices
of stride 28. The ps_1_0 shader has tex/mul/dp3/mul operations. State blocks,
render targets and viewport changes are also required. A second capture records
3160 Present frames. Replaying call 215150 (frame 2100) produced the original
640x480 title/menu with Story Mode, Arcade Mode, Practice Mode and other items.
The local reference is artifacts/iteration43/pc-reference-0000215150.png in the
parent workspace. This proves original-game startup on Windows, not on Vita.
Trace files contain
the user's game data and remain local, excluded from the public repository.

## Next implementation order

1. Guest window-creation callback dispatcher and persistent window state.
2. Bridge Direct3DCreate8/CreateDevice and owned COM interface/vtable objects.
3. Texture allocation/upload, transformed textured triangles, blending and
   presentation against the PC trace. Add the observed state-block/offscreen
   operations and shader as necessary for the title sequence.
4. Compare an original game frame on Vita with the Windows reference; only
   then measure sustained frame time and optimize.

Install 01.47 / T075VITA1 and confirm ITERATION 43,
build_id=iteration43-batch-checks-r1. Collect iteration43.log,
iteration43-runtime.log and iteration43-watchdog.log. The 300 previous
contract checks are retained; compilation does not certify hardware results.

References:
- https://github.com/amayra/arc_conv/blob/master/arc_conv/arc_th075.asm
- https://github.com/apitrace/apitrace/blob/master/docs/USAGE.markdown
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadicona
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassexa
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa
