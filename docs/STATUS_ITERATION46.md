# Iteration 46: original startup only, window paint and mounted file access

## Hardware evidence from 45 R2

The package installed. DAT frame decoding/presentation and all six original
creation/visibility callbacks passed on Vita. ShowWindow used SW_SHOWDEFAULT
(10). Main stopped at USER32 UpdateWindow, return 00602C8D. Startup took
29554256 us and the watchdog disarmed. The game title is still unverified.

## Changes

The 300-case synthetic batch is removed from the run and the build target.
No replacement test batch is added. Existing minimal CPU/IAT/heap/thread
preflight remains. Successful import logs are compact; full register/SEH
snapshots are retained at entry, unsupported imports and contract failures.
The screen shows the stopping API instead of a batch counter.

- UpdateWindow tracks the invalid client region and dispatches original
  WM_PAINT synchronously. The observed WndProc delegates to DefWindowProcA,
  which validates the region against the owned white-brush surface. A clean
  region returns without another callback. This is native window state,
  not Direct3D game rendering.
- CoInitialize(NULL) tracks STA registration/counts per guest TIB, returning
  S_OK first and S_FALSE for repeats. CoUninitialize balances the count.
  COM servers, marshaling and general CoCreateInstance are not implemented.
- CreateFileA read-only OPEN_EXISTING, GetFileAttributesA, GetFileSize,
  SetFilePointer, synchronous ReadFile, GetFileType and CloseHandle operate
  on owned native file objects under ux0:data/TH075Vita. Paths cannot escape
  the application mount. Reads copy in 4096-byte chunks into guest memory.
  Individual requests above 4 MiB, overlapped access, writes and mappings
  remain explicit unsupported boundaries. Each FILE is closed on teardown.
- Main resumes with the original startup instruction budget after window
  callbacks; the independent 60-second watchdog stays armed.

Version 01.50 / T075VITA1 / build_id=iteration46-boot-startup-r1.
The local VPK keeps the icon and updated 46/01.50 LiveArea art, with every PNG
forced to 8-bit indexed encoding. Public CI still omits local game artwork.

## Another Windows pass and the concrete graphics gap

The previously captured original D3D8 trace was replayed again through title
call 215150. A new menu frame was saved locally under artifacts/iteration46.
This is a replay of captured original calls, not a new capture of the EXE.
It validates the graphics reference, not execution on Vita.

The new summary records 2100 Present calls, 177 texture creations, 175
LockRect/UnlockRect uploads and 43238 DrawPrimitiveUP calls before that title
frame. It contains 36 distinct call names (including memcpy); therefore
35 are Direct3D API functions. The required path includes COM vtables,
adapter/device setup, texture/surface ownership, render state, transformed
triangles, offscreen render targets, state blocks, viewports and presentation.
The pixel shader is created but this segment has no SetPixelShader call;
its actual activation needs further inspection before treating it as a
requirement for the first title frame.

Another capture alone cannot provide those missing Vita implementations.
The next stages are:

1. Confirm UpdateWindow/COM and real file requests return on hardware.
2. Reach original Direct3DCreate8 and create owned guest interface/vtable
   objects backed by a Vita renderer; do not return dummy D3D_OK objects.
3. Feed textures and DrawPrimitiveUP from the original EXE into that renderer,
   including observed offscreen/state-block operations.
4. Present an original game frame and then validate the menu/input loop.

Until stage 4, a static DAT thumbnail or LiveArea photo does not count as boot.
The number of remaining iterations cannot be inferred from the Windows trace.

## Hardware run

Keep the Japanese TH075.exe and th075.dat in the data directory, with the
user's existing configuration/resources. Confirm ITERATION 46 BOOT STARTUP
and the R1 build ID, then send iteration46.log, iteration46-runtime.log and
iteration46-watchdog.log. Expected progress is
startup_window_update=guest_callbacks_completed, followed by later COM/file
calls and the actual next unsupported API. No batch_passed output is expected.

Host compilation and package inspection succeeded; new runtime paths need
validation on Vita. Local reference files and generated artwork stay outside
the public repository.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
- https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize
- https://github.com/apitrace/apitrace/blob/master/docs/USAGE.markdown

LiveArea label edit used built-in imagegen: preserve the existing background
and replace only the bottom-left label with ITERACION 46 / VERSION 01.50.
