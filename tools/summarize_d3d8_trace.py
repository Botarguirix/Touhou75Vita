"""Summarize an existing original-game apitrace text dump before a target frame."""
import argparse
import collections
import json
import re
from pathlib import Path

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('dump', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--last-call', type=int, default=215150)
    args = parser.parse_args()
    counts = collections.Counter()
    first = {}
    frames = 0
    with args.dump.open(encoding='utf-8', errors='replace') as source:
        for line in source:
            match = re.match(r'^(\d+) ([A-Za-z0-9_:]+)\(', line)
            if not match:
                continue
            call, function = int(match[1]), match[2]
            if call > args.last_call:
                continue
            counts[function] += 1
            first.setdefault(function, call)
            if function == 'IDirect3DDevice8::Present':
                frames += 1
    summary = dict(last_call=args.last_call, presents=frames, unique_functions=len(counts),
                   functions=[dict(name=name, calls=counts[name], first_call=first[name])
                              for name in sorted(counts)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, indent=2), encoding='utf-8')
    print('Present frames', frames, 'API functions', len(counts))
    for item in summary['functions']:
        print(item['name'], item['calls'], 'first', item['first_call'])
