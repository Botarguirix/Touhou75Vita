"""Verify extracted payloads and compare system PNG pixels to actual D3D8 uploads.

Input: extraction roots base/patch/music and an apitrace dump made with --blobs.
No proprietary data is embedded. Matches compare logical image pixels; padding
outside the image is deliberately excluded and is reported as such.
"""
import argparse
import hashlib
import json
import re
import wave
from collections import Counter, defaultdict
from pathlib import Path
from PIL import Image, ImageChops


def verify(root, dump):
    active, raw, music = {}, [], []
    for archive in ('base', 'patch', 'music'):
        folder = root / archive
        records = json.loads((folder / 'extraction.json').read_text(encoding='utf-8'))
        for record in records:
            path = folder.joinpath(*record['name'].split('/'))
            payload = path.read_bytes()
            if len(payload) != record['size'] or hashlib.sha256(payload).hexdigest() != record['sha256']:
                raise ValueError(f'extracted payload mismatch: {path}')
            raw.append(dict(archive=archive, name=record['name'], sha256=record['sha256']))
            if record['frames']:
                if record.get('decode_boundary'):
                    raise ValueError(f'partial image decode: {path}')
                for frame in record['frames']:
                    with Image.open(frame['file']) as image:
                        if hashlib.sha256(image.tobytes()).hexdigest() != frame['pixel_sha256']:
                            raise ValueError(f'PNG pixel mismatch: {frame["file"]}')
                active[record['name']] = dict(archive=archive, **record)
            if path.suffix.lower() == '.wav':
                with wave.open(str(path), 'rb') as sound:
                    music.append(dict(name=record['name'], channels=sound.getnchannels(),
                                      rate=sound.getframerate(), sample_width=sound.getsampwidth(),
                                      frames=sound.getnframes()))
    candidates = defaultdict(list)
    for name, record in active.items():
        for i, frame in enumerate(record['frames']):
            with Image.open(frame['file']) as image:
                rgba = image.convert('RGBA')
            width, height = rgba.size
            allocated = (1 << (width - 1).bit_length(), 1 << (height - 1).bit_length())
            keyed = None
            if frame['depth'] == 24:
                red, green, blue = rgba.convert('RGB').split()
                alpha = ImageChops.lighter(ImageChops.lighter(red, green), blue).point(lambda v: 255 if v else 0)
                keyed = rgba.copy()
                keyed.putalpha(alpha)
            candidates[allocated].append((name, record['archive'], i, rgba, keyed))
    textures, locked, matches, uploads, black_surfaces = {}, None, [], [], []
    for line in dump.read_text(encoding='utf-8-sig').splitlines():
        create = re.search(r'CreateTexture\(.*Width = (\d+), Height = (\d+).*Format = (D3DFMT_\w+).*ppTexture = &(0x[\da-f]+)', line)
        if create:
            width, height, fmt, pointer = create.groups()
            textures[pointer] = (int(width), int(height), fmt)
        lock = re.search(r'IDirect3DTexture8::LockRect\(this = (0x[\da-f]+).*Pitch = (\d+)', line)
        if lock:
            locked = (lock[1], int(lock[2]))
        copy = re.search(r'^(\d+) memcpy\(.*src = blob\("([^"]+)"\)', line)
        if not copy or not locked or locked[0] not in textures:
            continue
        call, file = copy.groups()
        width, height, fmt = textures[locked[0]]
        pitch = locked[1]
        payload = (dump.parent / file).read_bytes()
        unit = 2 if fmt == 'D3DFMT_A1R5G5B5' else 4
        if fmt not in ('D3DFMT_A1R5G5B5', 'D3DFMT_A8R8G8B8', 'D3DFMT_X8R8G8B8'):
            raise ValueError(f'unsupported upload format: {fmt}')
        if len(payload) != pitch * height or pitch < width * unit:
            raise ValueError(f'unexpected upload layout at call {call}')
        upload = dict(call=int(call), width=width, height=height, format=fmt,
                      pitch=pitch, bytes=len(payload), blob_sha256=hashlib.sha256(payload).hexdigest())
        uploads.append(upload)
        if fmt == 'D3DFMT_A8R8G8B8' and (width, height) == (1024, 512) and pitch >= 640 * 4:
            black = (b'\0\0\0\xff' * 640 + bytes(pitch - 640 * 4)) * 480 + bytes(pitch * (height - 480))
            if payload == black:
                black_surfaces.append(dict(call=int(call), logical_width=640, logical_height=480,
                                           pattern='opaque black rectangle with zero texture padding',
                                           origin='not matched to DAT; generation inferred, not proven'))
        cropped_hashes = {}
        for name, archive, index, image, keyed in candidates[(width, height)]:
            iw, ih = image.size
            if (iw, ih) not in cropped_hashes:
                cropped = b''.join(payload[y * pitch:y * pitch + iw * unit] for y in range(ih))
                if unit == 4:
                    decoded = Image.frombytes('RGBA', (iw, ih), cropped, 'raw', 'BGRA')
                    decoded_bytes = decoded.convert('RGB').tobytes() if fmt == 'D3DFMT_X8R8G8B8' else decoded.tobytes()
                else:
                    expanded = bytearray()
                    for i in range(0, len(cropped), 2):
                        pixel = int.from_bytes(cropped[i:i + 2], 'little')
                        expanded.extend((((pixel >> 10) & 31) * 255 // 31,
                                         ((pixel >> 5) & 31) * 255 // 31,
                                         (pixel & 31) * 255 // 31, 255 if pixel & 32768 else 0))
                    decoded_bytes = bytes(expanded)
                cropped_hashes[(iw, ih)] = hashlib.sha256(decoded_bytes).digest()
            expected_bytes = image.convert('RGB').tobytes() if fmt == 'D3DFMT_X8R8G8B8' else image.tobytes()
            equal = cropped_hashes[(iw, ih)] == hashlib.sha256(expected_bytes).digest()
            policy = 'stored_format'
            if not equal and keyed is not None and fmt == 'D3DFMT_A8R8G8B8':
                equal = cropped_hashes[(iw, ih)] == hashlib.sha256(keyed.tobytes()).digest()
                policy = 'original_24bit_black_color_key'
            if equal:
                matches.append(dict(resource=name, archive=archive, frame=index, call=int(call),
                                    format=fmt, logical_width=iw, logical_height=ih,
                                    alpha_policy=policy,
                                    comparison='exact logical pixels; texture padding excluded'))
    return dict(raw_verified=len(raw), music_verified=music,
                active_system_frames=sum(len(r['frames']) for r in active.values()),
                upload_formats=dict(Counter(u['format'] for u in uploads)),
                uploads=uploads, matches=matches,
                matched_upload_count=len(set(m['call'] for m in matches)),
                black_surface_uploads=black_surfaces,
                unmatched_upload_calls=sorted(set(u['call'] for u in uploads) - set(m['call'] for m in matches)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('extracted', type=Path)
    parser.add_argument('blob_dump', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report = verify(args.extracted, args.blob_dump)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('verified payloads', report['raw_verified'], 'active system frames', report['active_system_frames'],
          'uploads', len(report['uploads']), 'pixel matches', len(report['matches']))
    print('title matches', [m for m in report['matches'] if m['resource'].endswith('/title.dat')])
