"""Encode original imagegen masters once into Vita's PNG8 artwork profile.

Keep the source art outside Git. Never use an older package PNG as input:
each iteration starts with the full-resolution masters to avoid degradation.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageOps
from prepare_livearea import encode


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(background_path, icon_path, output, iteration, version):
    if iteration < 1:
        raise ValueError('iteration must be positive')
    sources = {}
    for name, path in [('background', background_path), ('icon', icon_path)]:
        with Image.open(path) as image:
            image.load()
            if min(image.size) < 500:
                raise ValueError(f'{name}: expected a full-resolution source master')
            if 'A' in image.getbands() and image.getchannel('A').getextrema() != (255, 255):
                raise ValueError(f'{name}: LiveArea artwork must be opaque')
            sources[name] = dict(path=str(path.resolve()), sha256=sha256(path), size=image.size)
    with Image.open(background_path) as source:
        background = source.convert('RGB')
    with Image.open(icon_path) as source:
        icon = source.convert('RGB')
    contents = output / 'livearea' / 'contents'
    outputs = [('pic0.png', background, (960, 544)),
               ('livearea/contents/bg0.png', background, (840, 500)),
               ('icon0.png', icon, (128, 128))]
    for name, image, size in outputs:
        encode(ImageOps.fit(image, size, Image.Resampling.LANCZOS), size, output / name)
    # The launch-gate image uses the same master, centered without stretching.
    gate = Image.new('RGB', (280, 158), icon.getpixel((0, 0)))
    fitted = ImageOps.contain(icon, (140, 140), Image.Resampling.LANCZOS)
    gate.paste(fitted, ((280 - fitted.width) // 2, (158 - fitted.height) // 2))
    encode(gate, gate.size, contents / 'startup.png')
    names = [item[0] for item in outputs] + ['livearea/contents/startup.png']
    (contents / 'template.xml').write_text(f'''<?xml version="1.0" encoding="utf-8"?>
<livearea style="psmobile" format-ver="01.00" content-rev="{iteration}">
  <livearea-background><image>bg0.png</image></livearea-background>
  <gate><startup-image>startup.png</startup-image></gate>
</livearea>
''', encoding='utf-8', newline='\n')
    result = dict(iteration=iteration, version=version,
                  source='image_gen built-in; full-resolution original masters encoded once',
                  sources=sources,
                  outputs={name: sha256(output / name) for name in names})
    (output / 'provenance.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('background', type=Path)
    parser.add_argument('icon', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--iteration', type=int, required=True)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    prepare(args.background, args.icon, args.output, args.iteration, args.version)
