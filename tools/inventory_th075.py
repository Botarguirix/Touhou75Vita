"""Read-only TH075 archive inventory; see amayra/arc_conv arc_th075.asm."""
import argparse
import collections
import json
import struct
from pathlib import Path

def inventory(path):
    length = path.stat().st_size
    with path.open('rb') as stream:
        count_bytes = stream.read(2)
        if len(count_bytes) != 2:
            raise ValueError('missing entry count')
        count, = struct.unpack('<H', count_bytes)
        if not count or 2 + count * 108 > length:
            raise ValueError('invalid directory length')
        table = bytearray(stream.read(count * 108))
        key = delta = 0x64
        for index in range(len(table)):
            table[index] ^= key
            key = (key + delta) & 255
            delta = (delta + 0x4D) & 255
        entries = []
        for index in range(count):
            record = table[index * 108:(index + 1) * 108]
            raw_name = bytes(record[:100]).split(b'\0', 1)[0]
            name = raw_name.decode('cp932', errors='strict')
            size, offset = struct.unpack_from('<II', record, 100)
            if offset < 2 + count * 108 or offset + size > length:
                raise ValueError(f'entry outside archive: {name}')
            stream.seek(offset)
            signature = stream.read(min(size, 16)).hex()
            entries.append(dict(name=name, size=size, offset=offset, signature=signature))
    return dict(archive=str(path), size=length, count=count,
                extensions=dict(collections.Counter(Path(e['name']).suffix.lower() for e in entries)),
                entries=entries)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    results = []
    for name in ('th075.dat', 'th075b.dat', 'th075bgm.dat'):
        path = args.directory / name
        if path.exists():
            results.append(inventory(path))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    for result in results:
        print(Path(result['archive']).name, result['count'], result['extensions'])
