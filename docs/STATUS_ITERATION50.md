# Iteration 50: owned x86 Direct3D 8 root interface

## Hardware evidence from 49

Original logging successfully created/reopened log.txt, sought to EOF,
wrote and flushed its messages, and closed handles. The game log contains
SystemDataCreate... / ...OK / DirectGraphicsInit.... Original main stopped
at d3d8.dll!Direct3DCreate8, SDKVersion=220, return 00401138.
Startup took 5318277 us, compared with 35140570 us in 48. The watchdog
disarmed and there was no CPU fault or exhausted budget.

## Changes

Serve the known SDK 220 factory by constructing an owned root object at
00AB9000 with vtable at 00AB9100. Its 16 methods have separate traps in
00BFE000..00BFE0F0, routed through the existing main trap handler. The vtable
is read back before returning the object. Root creation uses stdcall cleanup
8 and validates EAX, EIP, ESP and preserved registers.

QueryInterface supports the exact IUnknown and IDirect3D8 IIDs and increments
the native reference count. AddRef/Release balance ownership, with dead
objects invalidated and overflow guarded. This root is an actual owned COM
interface; it is not a rendering device. All other methods stop explicitly
before emulating a result or mutating guest output. Method frames validate
the owned this pointer, live reference count, vtable and bounded main stack.

Static inspection establishes that the original EXE's first virtual call
is slot 13 (offset 0x34), GetDeviceCaps(adapter=0, type=HAL, pCaps), return
00401176. The new handler records those parameters and the readable
212-byte output range, then stops with IDirect3D8::GetDeviceCaps as the
boundary. It does not return fabricated hardware limits or claim D3D_OK.
The following original calls are GetAdapterDisplayMode and CreateDevice.

The existing apitrace dump does not contain that GetDeviceCaps call,
although the original EXE disassembly proves it exists. Therefore the
Windows graphics-call summary alone is insufficient as a complete interface
inventory. Both original control flow and capture are used for integration.

## Remaining graphics work and hardware run

Version 01.54 / T075VITA1 / build_id=iteration50-boot-startup-r1.
Install the local VPK, then send iteration50.log, iteration50-runtime.log,
iteration50-watchdog.log and log.txt. Expected evidence is
startup_d3d8_root=owned, startup_d3d8_vtable_readback=passed,
startup_d3d8_abi=passed and the original GetDeviceCaps call.

The next implementation is a renderer with owned device/surface/texture
objects and supported capability reporting, followed by original texture
uploads, transformed triangles, render states and Present. This package
does not implement a device, claim GPU integration or display an original
rendered game frame. The DAT preview and LiveArea artwork remain previews.

Interface layout/signature references (used as ABI documentation, not copied
implementation code):
- [Wine IDirect3D8 declaration](https://github.com/wine-mirror/wine/blob/master/include/d3d8.h)
- [Wine D3DCAPS8 declaration](https://github.com/wine-mirror/wine/blob/master/include/d3d8caps.h)

VitaSDK compilation succeeded. Package inspection confirmed ZIP CRCs,
APP_VER=01.54, TITLE_ID=T075VITA1 and four compatible 8-bit indexed PNGs.
The local LiveArea label was inspected. The new COM root, ABI and original
virtual dispatch still require the physical Vita run; no synthetic cases
were added or executed.
