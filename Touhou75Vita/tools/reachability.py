#!/usr/bin/env python3
import re,json,argparse
from pathlib import Path
INS=re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s+([a-z][a-z0-9.]*)\s*(.*)$',re.I)
COND=set('jo jno jb jae je jz jne jnz jbe ja jl jge jle jg js jns jp jnp jc jnc'.split())
TERM={'ret','retn','retf','iret','ud2','hlt'}
def parse(p):
 o=[]
 for l in Path(p).read_text(errors='replace').splitlines():
  m=INS.match(l)
  if m:o.append({'addr':int(m.group(1),16),'len':len(bytes.fromhex(m.group(2))),'mn':m.group(3).lower(),'op':m.group(4).strip()})
 return o
def target(op):
 m=re.search(r'\b0x([0-9a-f]{5,8})\b',op,re.I); return int(m.group(1),16) if m else None
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--asm',required=True);ap.add_argument('--entry',required=True);ap.add_argument('--out',required=True);a=ap.parse_args(); ins=parse(a.asm); by={x['addr']:x for x in ins}; entry=int(a.entry,16)
 seen=set(); stack=[entry]; edges=[]
 while stack:
  a0=stack.pop()
  if a0 in seen or a0 not in by:continue
  seen.add(a0); x=by[a0]; nxt=a0+x['len']; t=target(x['op']); mn=x['mn']
  if mn=='call':
   if t in by: edges.append([a0,t,'call']); stack.append(t)
   if nxt in by: edges.append([a0,nxt,'fallthrough']); stack.append(nxt)
  elif mn.startswith('j'):
   if t in by: edges.append([a0,t,'branch']); stack.append(t)
   if mn!='jmp' and nxt in by: edges.append([a0,nxt,'fallthrough']); stack.append(nxt)
  elif mn in TERM: pass
  else:
   if nxt in by: edges.append([a0,nxt,'fallthrough']); stack.append(nxt)
 out=Path(a.out);out.parent.mkdir(parents=True,exist_ok=True); out.write_text(json.dumps({'entry':hex(entry),'reachable_instructions':len(seen),'all_instructions':len(ins),'coverage':len(seen)/len(ins),'edges':len(edges),'reachable_addresses':[hex(x) for x in sorted(seen)]},indent=2)); print(json.dumps({'reachable_instructions':len(seen),'all_instructions':len(ins),'coverage':len(seen)/len(ins),'edges':len(edges)},indent=2))
if __name__=='__main__':main()
