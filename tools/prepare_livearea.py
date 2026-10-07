"""Encode local user-owned/generated art into Vita package PNG formats.

Use imagegen for the annotated background before invoking this format encoder.
No game art is stored in the public repository.
"""
import argparse
import hashlib
import struct
from pathlib import Path
from PIL import Image


def original_icon(exe):
    data = exe.read_bytes()
    if hashlib.sha256(data).hexdigest() != 'bd441e99075436e8dcad26f86ffcf5e6aac4f58b0ed3ee7442e4cb39d8e22c98':
        raise ValueError('unsupported EXE hash')
    nt, = struct.unpack_from('<I', data, 60)
    sections, = struct.unpack_from('<H', data, nt + 6)
    optional_size, = struct.unpack_from('<H', data, nt + 20)
    dib = None
    for section in range(sections):
        start = nt + 24 + optional_size + section * 40
        virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', data, start + 8)
        if rva <= 0x291190 and 0x291190 + 2216 <= rva + raw_size:
            offset = raw + 0x291190 - rva
            dib = data[offset:offset + 2216]
            break
    if dib is None or struct.unpack_from('<IIIHHI', dib) != (40, 32, 64, 1, 8, 0):
        raise ValueError('unexpected ICON1 bitmap')
    pixels = bytearray()
    for y in range(128):
        for x in range(128):
            row = 31 - y // 4
            column = x // 4
            index = dib[1064 + row * 32 + column]
            b, g, r, _ = dib[40 + 4 * index:44 + 4 * index]
            if dib[2088 + row * 4 + column // 8] & (128 >> (column % 8)):
                r, g, b = 13, 19, 32
            pixels.extend((r, g, b))
    return Image.frombytes('RGB', (128, 128), bytes(pixels))


def encode(image, dimensions, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.convert('RGB').resize(dimensions, Image.Resampling.LANCZOS).quantize(
        colors=256, method=Image.Quantize.MEDIANCUT).save(destination, bits=8, optimize=False)
    # Small palettes otherwise cause Pillow to select 1/2/4-bit PNGs. Vita's
    # LiveArea loader requires the 8-bit indexed profile even for tiny icons.
    header = destination.read_bytes()[16:29]
    width, height, bits, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', header)
    if (width, height) != dimensions or (bits, color, compression, filtering, interlace) != (8, 3, 0, 0, 0):
        raise ValueError(f'incompatible LiveArea PNG: {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('annotated_background', type=Path)
    parser.add_argument('screenshot', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    contents = args.output / 'livearea' / 'contents'
    encode(original_icon(args.exe), (128, 128), args.output / 'icon0.png')
    background = Image.open(args.annotated_background)
    encode(background, (840, 500), contents / 'bg0.png')
    encode(background, (960, 544), args.output / 'pic0.png')
    encode(Image.open(args.screenshot), (280, 158), contents / 'startup.png')
    (contents / 'template.xml').write_text('''<?xml version="1.0" encoding="utf-8"?>
<livearea style="psmobile" format-ver="01.00" content-rev="45">
  <livearea-background><image>bg0.png</image></livearea-background>
  <gate><startup-image>startup.png</startup-image></gate>
</livearea>
''', encoding='utf-8', newline='\n')
    for path in args.output.rglob('*.png'):
        with Image.open(path) as image:
            print(path.name, image.size, image.mode)
