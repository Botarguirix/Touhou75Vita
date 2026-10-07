# Iteration 45: original DAT image decoding and LiveArea identity

## Hardware iteration 44

The original WndProc completed WM_NCCREATE, WM_NCCALCSIZE and WM_CREATE.
The EXE itself set 0068D674 to 1. CreateWindowExA returned an owned handle;
main reached ShowWindow at return address 00602C80. All 300 retained checks
passed; elapsed startup was 28931294 us and the watchdog disarmed.

## Resource research and extraction

Two primary implementations confirm TH075's archive directory:
thtk's thdat105.c (version 75) and arc_unpacker's pak1 archive decoder.
arc_unpacker's pak1-gfx and pak1-sfx readers identify image and sound containers.
This iteration implements separate bounded readers rather than copying source.

The system extraction produced 17 files and 272 PNG frames with no image
decoding boundaries in those 16 image containers. title.dat has four frames;
its first is the 640x480 shrine background visible under the original menu.
The header includes palette count and per-frame dimensions/depth/encoded size.
Image payloads are run/color pairs. Tools support 8/16/24/32-bit images; the
native Vita preview deliberately supports only the observed 640x480 24/32-bit
first title frame with zero palettes and tight extents/run-count checks.

The extractor preserves .pat bytes; it does not decompile animation behavior
or scripts. SFX metadata is researched but the PCM reconstruction/playback
bridge is not implemented in this iteration. The full asset corpus and the
EXE's original file/texture interfaces still need integration.

Local extraction (requires Pillow):

```sh
python tools/extract_th075.py /path/to/th075.dat /new/output/directory
```

The default prefix is data/system/. --prefix selects other resource groups.
It rejects unsafe paths and existing raw destinations. Original files and
decoded art remain in the parent workspace's artifacts/iteration45 directory.

## Vita integration and next startup boundary

- Read the archive directory and decode title frame 0 directly from
  ux0:data/TH075Vita/th075.dat. Missing DAT is optional for EXE diagnostics.
- Display the native decoded frame as a small DAT RESOURCE preview on the
  diagnostic screen. This is a resource integration milestone, not D3D output.
- ShowWindow supports normal/show/default in the observed logical desktop.
  Deferred original WndProc callbacks receive WM_SHOWWINDOW, WM_SIZE and
  WM_MOVE; the persistent window tracks visibility and prior-visible return
  semantics. Default messages are restricted to that callback scope.
- UpdateWindow and activation/painting/message-loop semantics remain pending;
  the EXE stops at the next unsupported import. Game boot is unverified.

## Local LiveArea package

Version 01.49, title ID T075VITA1, build_id=iteration45-batch-checks-r1.
The delivered local VPK includes the decoded original ICON1 for its bubble,
a title-screen-based annotated LiveArea background with ITERACION 45 and
VERSION 01.49 at bottom left, a startup image and loading picture.
The annotated background was prepared with the built-in imagegen tool from
the Windows capture, then encoded to indexed PNG. It is decorative artwork;
the unmodified Windows capture and DAT-decoded image remain local references.

All PNGs are palette encoded: icon 128x128, background 840x500, gate 280x158,
loading picture 960x544. template.xml uses the psmobile gate layout so the gate
occupies the right and the bottom-left version label remains available.

Generate local assets with tools/prepare_livearea.py (EXE, annotated image,
original screenshot, output directory), then configure VITA_LIVEAREA_DIR to
that output. The PNG files are excluded from Git. Public CI lacks these local
images and produces a diagnostic VPK without the LiveArea art. Use the local
download for the requested artwork; no EXE or DAT is bundled in either VPK.

Hardware acceptance is still pending: inspect dat_title_result,
screen_original_dat_title, startup_window_show and final startup_stop_import.
Keep TH075.exe and th075.dat in ux0:data/TH075Vita and send all iteration45 logs.

## References

- [TH075 archive implementation](https://github.com/thpatch/thtk/blob/master/thtk/thdat105.c)
- [PAK1 image format implementation](https://github.com/vn-tools/arc_unpacker/blob/master/src/dec/twilight_frontier/pak1_image_archive_decoder.cc)
- [PAK1 sound format implementation](https://github.com/vn-tools/arc_unpacker/blob/master/src/dec/twilight_frontier/pak1_audio_archive_decoder.cc)
- [VitaSDK LiveArea example](https://github.com/vitasdk/samples/tree/master/pretty_livearea)
- [ShowWindow contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow)

Imagegen prompt: preserve the attached original title-screen screenshot as
closely as possible, fit it into a landscape 840x500 canvas with black side
margins, and add only a small white bottom-left two-line label: ITERACION 45 /
VERSION 01.49. No added characters, artwork, device mockup or perspective.
