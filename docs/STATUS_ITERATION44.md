# Iteration 44: synchronous original window procedure callbacks

Hardware iteration 43 decoded ICON1, reused its owned handle, registered atom
C001 with original WndProc 00603650, and stopped at CreateWindowExA. All 300
previous service checks passed. Startup took 28686915 us; the watchdog disarmed.
The decoded icon was presented on the Vita diagnostic screen.

## Changes

- CreateWindowExA validates the observed hidden top-level 640x480 window,
  registered class/module and null owner/menu/parameter. Unsupported profiles
  still stop. A missing class returns NULL with error 1407.
- A persistent window object owns a 640x480 ABGR surface initialized with the
  registered white background brush. This is logical window state; no game
  image is presented from this surface yet.
- Window creation yields out of Cpu::run. The runner invokes the original
  procedure on the same guest thread, without recursively entering the CPU
  from an import trap. WM_NCCREATE, WM_NCCALCSIZE and WM_CREATE execute in order.
- Guest CREATESTRUCTA and RECT values live below the suspended import frame.
  The original procedure delegates the first two messages to a narrowly scoped
  DefWindowProcA implementation. The borderless logical profile has zero
  non-client margins. Other default messages remain unsupported.
- Each callback checks its return sentinel, stdcall stack cleanup, nonvolatile
  registers and current TIB. Main architectural state is restored between
  callbacks. The original WM_CREATE code must set byte 0068D674 to 1.
  The bridge never writes that success flag itself.
- Only after those checks does CreateWindowExA return the owned HWND and
  resume the original executable. The next unsupported import is recorded.
- The wall-clock watchdog is now 60 seconds: iteration 43 consumed 28.69 of
  the previous 30 seconds before window creation. The instruction budget
  remains bounded. This change does not constitute a performance improvement.

## Remaining work and acceptance

Window visibility/activation, the message loop, input and Direct3D 8 graphics
remain pending. This iteration deliberately stops at the next unsupported API;
it does not report game boot success. No additional synthetic cases are added.

Compilation and package inspection are host checks. Hardware acceptance needs
three startup_window_callback_result=passed records, original_create_flag=1,
window_creation=guest_callbacks_completed and a later startup_stop_import.
Install version 01.48 / T075VITA1; confirm ITERATION 44 and
build_id=iteration44-batch-checks-r1. Send iteration44.log,
iteration44-runtime.log and iteration44-watchdog.log.

Windows title/menu capture from iteration 43 remains the graphics reference;
there is no verified title frame on Vita yet.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa
- https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-nccreate
- https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-nccalcsize
- https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
