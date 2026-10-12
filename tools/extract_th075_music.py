"""Extract original BGM WAV files without converting or rewriting their chunks.

Existing destination files are verified byte-for-byte by SHA256, never replaced.
The archive and extracted music must remain outside Git.
"""
import argparse
import hashlib
import json
import wave
from pathlib import Path, PurePosixPath
from inventory_th075 import inventory


def extract(archive, output):
    root = output.resolve()
    root.mkdir(parents=True, exist_ok=True)
    report = []
    with archive.open('rb') as source:
        for entry in inventory(archive)['entries']:
            name = entry['name'].replace('\\', '/')
            relative = PurePosixPath(name)
            if relative.is_absolute() or '..' in relative.parts or ':' in name:
                raise ValueError('unsafe archive path')
            if not name.startswith('wave/bgm/') or relative.suffix.lower() != '.wav':
                continue
            path = root.joinpath(*relative.parts)
            if not path.resolve().is_relative_to(root):
                raise ValueError('destination outside root')
            source.seek(entry['offset'])
            data = source.read(entry['size'])
            if len(data) != entry['size'] or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
                raise ValueError(f'invalid WAV entry: {name}')
            digest = hashlib.sha256(data).hexdigest()
            if path.exists():
                if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
                    raise ValueError(f'existing destination differs: {path}')
                action = 'existing_verified'
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                with path.open('xb') as destination:
                    destination.write(data)
                if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
                    raise ValueError('output readback mismatch')
                action = 'extracted_verified'
            with wave.open(str(path), 'rb') as sound:
                report.append(dict(name=name, file=str(path), bytes=len(data), sha256=digest,
                                   channels=sound.getnchannels(), bits=8*sound.getsampwidth(),
                                   rate=sound.getframerate(), frames=sound.getnframes(),
                                   seconds=sound.getnframes()/sound.getframerate(),
                                   compression=sound.getcomptype(), action=action))
    if not report:
        raise ValueError('no BGM WAV files found')
    result = dict(archive=str(archive.resolve()), count=len(report),
                  bytes=sum(r['bytes'] for r in report),
                  seconds=sum(r['seconds'] for r in report), tracks=report)
    (root/'music-audit.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    lines = ['# Música original extraída de th075bgm.dat', '',
             'Archivos WAV originales, sin transcodificar; cada archivo coincide por SHA256 con su entrada del DAT.',
             'Se conservan todos los chunks originales. Los nombres de pistas no se han identificado por título.', '',
             '| Archivo | Duración | PCM |', '| --- | --- | --- |']
    for r in report:
        minutes, seconds = divmod(int(r['seconds']), 60)
        lines.append(f"| [{Path(r['file']).name}](<{Path(r['file']).as_posix()}>) | {minutes}:{seconds:02d} | {r['rate']} Hz / {r['bits']} bits / {r['channels']} canales |")
    (root/'music-index.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report = extract(args.archive, args.output)
    print(json.dumps({k: v for k, v in report.items() if k != 'tracks'}, indent=2))
    print('formats', sorted({(r['rate'], r['bits'], r['channels'], r['compression']) for r in report['tracks']}))
