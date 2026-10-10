"""Build local Vita artwork from a freshly decoded user-owned DAT portrait.

No generated game art is committed. Each PNG is rendered from the original
RGBA frame, then encoded once to Vita's indexed eight-bit PNG profile.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from inventory_th075 import inventory
from extract_th075 import images
from prepare_livearea import encode


def make_assets(archive, output, frame, iteration, version, font_path):
    entry = next(e for e in inventory(archive)['entries']
                 if e['name'].replace('\\', '/').lower() == 'data/system/selectchar.dat')
    with archive.open('rb') as stream:
        stream.seek(entry['offset'])
        data = stream.read(entry['size'])
    if len(data) != entry['size']:
        raise ValueError('short DAT read')
    portrait, info = next((image, meta) for index, image, meta in images(data, True) if index == frame)
    if portrait.size != (512, 512) or portrait.mode != 'RGBA':
        raise ValueError('expected original 512x512 RGBA portrait')
    output.mkdir(parents=True, exist_ok=True)
    portrait.save(output / 'dat-character-original.png')
    contents = output / 'livearea' / 'contents'

    def background(size):
        canvas = Image.new('RGBA', size, (13, 19, 32, 255))
        height = size[1] - 16
        scaled = portrait.resize((height, height), Image.Resampling.LANCZOS)
        canvas.alpha_composite(scaled, (0, size[1] - height))
        draw = ImageDraw.Draw(canvas)
        x = int(size[0] * .61)
        font = ImageFont.truetype(str(font_path), int(size[1] * .047))
        small = ImageFont.truetype(str(font_path), int(size[1] * .031))
        for y, label in [(size[1] * .16, 'TOUHOU 7.5'), (size[1] * .24, 'VITA')]:
            draw.text((x, int(y)), label, font=font, fill=(240, 244, 251, 255))
        draw.text((x, int(size[1] * .78)), f'ITERATION {iteration}', font=small, fill=(117, 201, 233, 255))
        draw.text((x, int(size[1] * .85)), f'VERSION {version}', font=small, fill=(114, 220, 159, 255))
        return canvas

    for size, destination in [((840, 500), contents / 'bg0.png'), ((960, 544), output / 'pic0.png')]:
        encode(background(size), size, destination)
    # Each crop comes from the original portrait, never from a previous PNG.
    icon = Image.new('RGBA', (256, 256), (13, 19, 32, 255))
    icon.alpha_composite(portrait.crop((112, 0, 368, 256)))
    encode(icon, (128, 128), output / 'icon0.png')
    gate = Image.new('RGBA', (280, 158), (13, 19, 32, 255))
    gate.alpha_composite(portrait.resize((158, 158), Image.Resampling.LANCZOS))
    ImageDraw.Draw(gate).text((175, 54), str(iteration), font=ImageFont.truetype(str(font_path), 37), fill='white')
    encode(gate, gate.size, contents / 'startup.png')
    (contents / 'template.xml').write_text(f'''<?xml version="1.0" encoding="utf-8"?>
<livearea style="psmobile" format-ver="01.00" content-rev="{iteration}">
  <livearea-background><image>bg0.png</image></livearea-background>
  <gate><startup-image>startup.png</startup-image></gate>
</livearea>
''', encoding='utf-8', newline='\n')
    result = dict(archive=str(archive), entry=entry, container_sha256=hashlib.sha256(data).hexdigest(),
                  frame=frame, pixel_sha256=hashlib.sha256(portrait.tobytes()).hexdigest(),
                  metadata=info, iteration=iteration, version=version,
                  source='original DAT frame freshly decoded; no generative edits',
                  outputs={p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in sorted(output.rglob('*.png'))})
    (output / 'provenance.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['entry', 'frame', 'iteration', 'version', 'source']}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--frame', type=int, default=6)
    parser.add_argument('--iteration', type=int, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--font', type=Path, required=True)
    args = parser.parse_args()
    make_assets(args.archive, args.output, args.frame, args.iteration, args.version, args.font)
