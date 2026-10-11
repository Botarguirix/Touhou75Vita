"""Analyze genuine EXE frame cycles and the 60 FPS deadline, from existing logs.

Raster/Present timings are subsets of the cycle. Remaining time includes guest,
other API bridges, timer work and diagnostics; it is not a pure JIT measurement.
"""
import argparse
import json
import math
import re
from pathlib import Path


def summarize(samples):
    if not samples:
        return dict(frames=0,mean_us=None,fps=None,p50_us=None,p95_us=None,
                    p99_us=None,within_60fps_budget=0,missed_60fps_budget=0)
    values=sorted(s['cycle_us'] for s in samples)
    count=len(values)
    total=sum(values)
    return dict(frames=count,mean_us=total/count,fps=count*1e6/total,
                p50_us=values[math.ceil(count*.50)-1],
                p95_us=values[math.ceil(count*.95)-1],
                p99_us=values[math.ceil(count*.99)-1],max_us=values[-1],
                within_60fps_budget=sum(v<=16667 for v in values),
                missed_60fps_budget=sum(v>16667 for v in values),
                draw_mean_us=sum(s['draw_us'] for s in samples)/count,
                present_mean_us=sum(s['present_us'] for s in samples)/count,
                remaining_mean_us=sum(s['remaining_us'] for s in samples)/count,
                timing_overlap_cycles=sum(s['timing_overlap'] for s in samples))


def analyze(text):
    samples=[];active=None;scene=None;excluded=0;incomplete=0
    for line in text.splitlines():
        match=re.match(r'startup_frame_scene=.*vtable:(0x[0-9A-Fa-f]+)',line)
        if match:
            scene=match[1].upper()
        if line.startswith('startup_frame_wait=blocked '):
            if active is not None:
                incomplete+=1
            active=dict(draw_us=0,present_us=0,presents=0,scene=scene)
        if active is None:
            continue
        match=re.match(r'startup_d3d8_draw_elapsed_us=(\d+)',line)
        if match:
            active['draw_us']+=int(match[1])
        match=re.match(r'startup_d3d8_present_elapsed_us=(\d+)',line)
        if match:
            active['present_us']+=int(match[1])
        if line.startswith('startup_d3d8_present=executed '):
            active['presents']+=1
        match=re.match(r'startup_frame_cycle_elapsed_us=(\d+) cycle:(\d+)',line)
        if match:
            duration=int(match[1])
            if active['presents']==1 and 0<duration<=210000000:
                rest=duration-active['draw_us']-active['present_us']
                samples.append(dict(cycle=int(match[2]),cycle_us=duration,
                                    draw_us=active['draw_us'],present_us=active['present_us'],
                                    remaining_us=max(0,rest),timing_overlap=rest<0,scene=active['scene']))
            else:
                excluded+=1
            active=None
    if active is not None:
        incomplete+=1
    return dict(target_fps=60,budget_us=16667,
                scope='timer_and_main_cycles_with_one_confirmed_present',
                remaining_scope='guest_other_bridges_timer_and_diagnostics_not_pure_jit',
                all=summarize(samples),
                menu=summarize([s for s in samples if s['scene']=='0X00658308']),
                excluded_no_multi_present_or_invalid=excluded,
                incomplete_cycles=incomplete,samples=samples)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=analyze(args.log.read_text(encoding='utf-8',errors='replace'))
    result['source']=str(args.log.resolve())
    data=json.dumps(result,indent=2)
    if args.output:
        args.output.write_text(data,encoding='utf-8')
    brief={key:value for key,value in result.items() if key!='samples'}
    print(json.dumps(brief,indent=2))
