# Iteration 51 / version 01.55 — graphics storage

## Evidence used

Physical iteration 50 reached IDirect3D8::GetDeviceCaps in 5.5 seconds,
with owned root/vtable and correct x86 stdcall returns. The fresh Windows
reference pass verified 286 extracted archive entries and matched 225/226
distinct texture uploads to system resources. See ORIGINAL_REFERENCE_PASS_51.md.

## New implementation

- Root GetDeviceCaps returns a 212-byte resource-only profile: 1024 texture
  width/height/aspect limits; raster, blend, filtering, vertex and pixel shader
  capability fields remain zero. These are partial bridge limits, not the
  Windows driver's 8192 limits or a claim of a complete D3D implementation.
- One logical 640x480, 60 Hz X8R8G8B8 display mode. Hardware/HAL creation
  returns D3DERR_NOTAVAILABLE. The original EXE's existing third attempt
  selects REF with SOFTWARE_VERTEXPROCESSING; no EXE instruction is patched.
- Owned device, 640x480 color and D16 depth storage; 97-slot device vtable.
  A8R8G8B8 and A1R5G5B5 one-level power-of-two textures through 1024x1024,
  default render-target or managed storage, and D16 surface allocations.
- Texture/surface ownership, descriptions, GetSurfaceLevel and bounded
  full-resource LockRect/UnlockRect. A single 4 MiB guest staging buffer
  copies uploaded bytes into native storage. Only one active lock is allowed.
  Pitch, bytes, FNV-1a and transparent/opaque/partial-alpha counts are logged.
  Uploaded alpha/channel bytes are preserved without an extra conversion.
- Observed startup render/texture-stage state and FVF 0x144 are stored.
  BeginScene/EndScene track pairing; full color/depth Clear writes storage.
- Original DAT diagnostic preview uses the verified 24-bit black color key.
  LiveArea retains the original EXE icon and game reference art, with 51/01.55
  in the lower-left badge. The art is a reference, not an EXE-rendered frame.

## Bounds and explicit missing operations

Native pixel storage is capped at 32 MiB and 512 resource records; allocation
failure returns D3DERR_OUTOFVIDEOMEMORY. Guest staging occupies 01400000–01800000,
after the existing 8 MiB guest heap. Root/device/resource records are separate.
COM AddRef and non-final Release are served; complete resource/device
destruction and device/texture QueryInterface remain stopping boundaries.
Subrect/multiple locks, mip chains and unobserved creation/state contracts stop.

DrawPrimitiveUP, Present, render-target switching, state blocks and shaders
remain unimplemented. This iteration can advance graphics initialization and
resource allocation/upload; it cannot display the original running menu yet.
Other Win32, DirectInput or audio boundaries may be reached first. The fresh
Windows trace used HAL with shaders enabled; this build deliberately exercises
the original REF/software/no-shader fallback and needs hardware observation.

No 300-case synthetic batch is re-enabled. Startup retains its existing
instruction-slice/progress bounds and independent watchdog.

## Delivery and hardware observation

Local VPK: artifacts/iteration51/Touhou75Vita-iteration51.vpk, version 01.55,
title ID T075VITA1. Build ID: iteration51-graphics-storage-r1.
Compile/package inspection does not establish execution on physical Vita.
The final VitaSDK build completed without compiler warnings; ZIP CRC, SFO
01.55/T075VITA1 and all four 8-bit indexed non-interlaced PNGs passed inspection.
VPK SHA256: bb1af8814dd44e1af9a949c4a65655f3b20027e8f81d031c9b00e263ef8484ef.

Collect iteration51.log, iteration51-runtime.log, iteration51-watchdog.log,
the original log.txt and a photo. Inspect the REF/software fallback, device
and texture allocation records, upload records when reached, the final
startup_stop_import/EIP and watchdog status. Success is a later recorded
original startup boundary with valid ABI, not the reference art on screen.

ABI reference: https://raw.githubusercontent.com/wine-mirror/wine/master/include/d3d8.h
