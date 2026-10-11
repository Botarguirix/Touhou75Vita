"""Extract one PCM effect from the user's TH075 PAK1 sound container.

Independent bounded reader; format reference: arc_unpacker pak1-audio.
Does not distribute resources or infer the effect's in-game purpose.
"""
import argparse
import hashlib
import json
import struct
import wave
from pathlib import Path
from inventory_th075 import inventory


def effect(data, wanted):
    if len(data) < 4:
        raise ValueError('truncated container')
    count, = struct.unpack_from('<I', data)
    if not 0 < count <= 4096 or not 0 <= wanted < count:
        raise ValueError('invalid count/index')
    cursor = 4
    selected = None
    for index in range(count):
        if cursor >= len(data):
            raise ValueError('truncated entry flag')
        flag = data[cursor]
        cursor += 1
        if not flag:
            continue
        if cursor + 22 > len(data):
            raise ValueError('truncated sound header')
        size, fmt, channels, rate, byte_rate, align, bits, extra = struct.unpack_from('<IHHIIHHH', data, cursor)
        cursor += 22
        if cursor + size > len(data):
            raise ValueError('sound outside container')
        if index == wanted:
            if fmt != 1 or channels not in (1, 2) or bits != 16 or extra or rate not in (22050, 44100, 48000):
                raise ValueError('unsupported PCM format')
            if align != channels * 2 or byte_rate != rate * align or not size or size % align or size > byte_rate * 2:
                raise ValueError('invalid PCM extent')
            selected = (data[cursor:cursor + size], channels, rate)
        cursor += size
    if cursor != len(data) or selected is None:
        raise ValueError('trailing bytes or missing selected effect')
    return selected


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--entry', default='wave/se.dat')
    parser.add_argument('--index', type=int, default=2)
    args = parser.parse_args()
    matches = [e for e in inventory(args.archive)['entries'] if e['name'].replace('\\', '/') == args.entry]
    if len(matches) != 1:
        raise ValueError('missing or duplicate archive entry')
    entry = matches[0]
    with args.archive.open('rb') as source:
        source.seek(entry['offset'])
        data = source.read(entry['size'])
    pcm, channels, rate = effect(data, args.index)
    samples = struct.unpack('<' + 'h' * (len(pcm) // 2), pcm)
    peak = max(abs(s) for s in samples)
    if not peak:
        raise ValueError('selected effect is silent')
    if args.output.exists():
        raise ValueError('output already exists')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(args.output), 'wb') as target:
        target.setnchannels(channels)
        target.setsampwidth(2)
        target.setframerate(rate)
        target.writeframes(pcm)
    # Read-back verifies packaging of the unchanged decoded PCM.
    with wave.open(str(args.output), 'rb') as check:
        if check.readframes(check.getnframes()) != pcm:
            raise ValueError('WAV PCM readback mismatch')
    report = dict(archive=str(args.archive), entry=args.entry, index=args.index,
                  container_sha256=hashlib.sha256(data).hexdigest(), pcm_sha256=hashlib.sha256(pcm).hexdigest(),
                  channels=channels, bits=16, sample_rate=rate, pcm_bytes=len(pcm),
                  frames=len(pcm)//(channels*2), seconds=len(pcm)/(rate*channels*2),
                  peak=peak, nonzero_samples=sum(s != 0 for s in samples), wav_readback='passed')
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
