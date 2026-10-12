# Original EXE reference pass 51 (research, no new VPK)

## Physical Vita result from 50

The owned IDirect3D8 root, vtable readback and stdcall cleanup passed.
The original virtual call reached GetDeviceCaps: adapter 0, HAL type 1,
output 009FEB04, return 00401176. Startup took 5509642 us; watchdog disarmed.
The native renderer remains explicitly unimplemented. The game log reached
SystemDataCreate...OK and DirectGraphicsInit, matching the current boundary.

## Fresh original Windows execution

The user's known Japanese EXE was run again through apitrace in the existing
isolated pc-game copy, then closed. This was a new execution/capture, not just
replay of the old trace. New trace: artifacts/iteration51/original-fresh.trace.
Original log confirms graphics, input and sound initialization and completion
of SystemDataInit. Windows reports maximum texture dimensions/aspect 8192;
these are Windows driver capabilities, not valid Vita capability values.

Replaying the fresh trace produced the complete original title/menu frame
at call 102934. Up to that call, the trace records 1054 Present calls,
228 texture creations, 226 texture uploads and 19941 DrawPrimitiveUP calls.
The dump contains 35 D3D API names plus memcpy. Static inspection and Vita
confirm GetDeviceCaps even though apitrace omits it from the dump.

The original log enables the pixel-shader path and the trace creates ps_1_0
and sets a constant. There is no SetPixelShader call in this capture segment.
This does not prove shader activation is needed for the first title frame;
shader creation/lifetime and the later activation path still need support
or a validated compatibility implementation.

## Archive extraction and pixel verification

All 286 raw entries were extracted: 215 base, 37 patch and 34 music files.
Every extracted payload was read back and checked against its source SHA256.
The 34 WAV files were also opened to validate their audio headers. Character
PAT/SCE and sound-effect DAT payloads were extracted, but their internal
formats were not decompiled in this pass.

System image decoding produced 272 base PNG frames and 50 patch PNG frames,
with no partial RLE failures. Applying patch overrides leaves 275 effective
system frames. Raw format conversion and game upload conversion are distinct:
the observed original 24-bit upload treats pure RGB black as transparent.
The extractor now provides --game-alpha for that behavior while retaining
the original stored-format decoding default.

The verification tool compares those effective images to actual apitrace
texture upload blobs, including pitch, allocation dimensions and formats.
225 of 226 upload calls match exact logical pixels from system resources.
This excludes texture padding from the image comparison. All four title
layers match, including the 640x480 background after black-color-key handling.
There are 227 frame-to-upload matches because some identical frames map to
the same upload. This is not 227 distinct texture upload calls.

The remaining call 61 is exactly an opaque-black 640x480 rectangle in a
1024x512 texture with zero padding. It has no DAT match; generation by the
EXE is an inference, not a proven source mapping. Uploaded formats comprise
137 A8R8G8B8 textures and 89 A1R5G5B5 textures, maximum 1024x1024.
The 8192 Windows limit should therefore not be copied as a Vita requirement.

## Concrete improvements and remaining work

- Extraction manifests now record source offsets, payload hashes and decoded
  pixel hashes. --decode-prefix allows extraction of all payloads while
  focusing image work on boot/menu resources.
- The new verification tool applies patch precedence, checks WAV headers,
  compares uploaded texture pixels and distinguishes black-color-key behavior.
- Render integration now has verified channel order, alpha, 16-bit format,
  texture pitch and padding requirements. Preserve black-key behavior for
  observed 24-bit uploads, stored alpha for 32-bit frames, and BGRA5551 alpha
  for indexed/16-bit textures; do not make all assets opaque.
- Next native work remains truthful GetDeviceCaps, display mode, an owned
  device/surfaces/textures, render-target switching, state blocks and original
  transformed textured triangles through Present. Resource extraction alone
  does not implement that renderer or replace game logic from PAT/SCE files.

No new VPK is produced in this reference pass. Version 01.54/iteration50
remains the current physical Vita package. Local game files, traces, blobs,
PNGs and reports stay outside Git; only tools and documentation are committed.
