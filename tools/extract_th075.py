"""Extract user-owned TH075 system containers and decode their RLE images.

Format references: vn-tools/arc_unpacker twilight-frontier/pak1-gfx;
thpatch/thtk thdat105.c. This is a separate bounded Python implementation.
"""
import argparse
import json
import struct
from pathlib import Path, PurePosixPath
from PIL import Image
from inventory_th075 import inventory


def images(data):
    if not data:
        raise ValueError('empty image container')
    palettes = data[0]
    cursor = 1 + 512 * palettes
    if cursor > len(data):
        raise ValueError('truncated palettes')
    palette = data[1:513] if palettes else None
    index = 0
    while cursor < len(data):
        if len(data) - cursor < 17:
            raise ValueError('truncated frame header')
        width, height, storage_width, depth, size = struct.unpack_from('<IIIBI', data, cursor)
        cursor += 17
        if not (0 < width <= 4096 and 0 < height <= 4096 and width * height <= 16777216):
            raise ValueError('invalid image dimensions')
        if depth not in (8, 16, 24, 32) or cursor + size > len(data):
            raise ValueError('invalid frame encoding/extent')
        unit = 4 if depth >= 24 else depth // 8
        payload = memoryview(data)[cursor:cursor + size]
        raw = bytearray()
        p = 0
        expected = width * height * unit
        while p < len(payload):
            if p + unit * 2 > len(payload):
                raise ValueError('truncated RLE pair')
            run = int.from_bytes(payload[p:p + unit], 'little')
            color = bytes(payload[p + unit:p + unit * 2])
            if not run or len(raw) + run * unit > expected:
                raise ValueError('invalid RLE run')
            raw.extend(color * run)
            p += unit * 2
        if len(raw) != expected:
            raise ValueError('incomplete frame')
        if depth in (24, 32):
            image = Image.frombytes('RGBA' if depth == 32 else 'RGB', (width, height), bytes(raw),
                                    'raw', 'BGRA' if depth == 32 else 'BGRX')
        else:
            rgba = bytearray()
            for pixel in range(width * height):
                if depth == 8:
                    if palette is None:
                        raise ValueError('indexed frame without palette')
                    value = int.from_bytes(palette[2 * raw[pixel]:2 * raw[pixel] + 2], 'little')
                else:
                    value = int.from_bytes(raw[2 * pixel:2 * pixel + 2], 'little')
                rgba.extend((((value >> 10) & 31) * 255 // 31,
                             ((value >> 5) & 31) * 255 // 31,
                             (value & 31) * 255 // 31, 255 if value & 32768 else 0))
            image = Image.frombytes('RGBA', (width, height), bytes(rgba))
        yield index, image, dict(width=width, height=height, storage_width=storage_width,
                                 depth=depth, compressed_bytes=size)
        cursor += size
        index += 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--prefix', default='data/system/')
    args = parser.parse_args()
    root = args.output.resolve()
    report = []
    with args.archive.open('rb') as source:
        for entry in inventory(args.archive)['entries']:
            name = entry['name'].replace('\\', '/')
            path = PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or ':' in name:
                raise ValueError('unsafe archive path')
            if not name.startswith(args.prefix):
                continue
            destination = root.joinpath(*path.parts)
            if not destination.resolve().is_relative_to(root):
                raise ValueError('archive path escaped output')
            if destination.exists():
                raise ValueError(f'output already exists: {destination}')
            source.seek(entry['offset'])
            data = source.read(entry['size'])
            if len(data) != entry['size']:
                raise ValueError('short archive read')
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            item = dict(name=name, size=len(data), frames=[])
            if name.endswith('.dat'):
                try:
                    for index, image, metadata in images(data):
                        png = destination.with_name(f'{destination.stem}-{index:04d}.png')
                        image.save(png)
                        item['frames'].append(dict(file=str(png), **metadata))
                except ValueError as error:
                    item['decode_boundary'] = str(error)
            report.append(item)
    root.mkdir(parents=True, exist_ok=True)
    (root / 'extraction.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('containers', len(report), 'decoded frames', sum(len(r['frames']) for r in report))
    for item in report:
        print(item['name'], len(item['frames']), item.get('decode_boundary', 'decoded'))
