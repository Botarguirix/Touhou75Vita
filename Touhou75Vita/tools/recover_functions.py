#!/usr/bin/env python3
import re,json,struct,argparse
from pathlib import Path
INS=re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s+([a-z][a-z0-9.]*)\s*(.*)$',re.I)
HEX_TARGET=re.compile(r'^(?:0x)?([0-9a-f]+)(?:\s|$)',re.I)
COND=set('jo jno jb jae je jz jne jnz jbe ja jl jge jle jg js jns jp jnp jc jnc'.split())
RET=set('ret retn retf iret'.split())

def parse_asm(path):
    ins=[]
    for line in Path(path).read_text(errors='replace').splitlines():
        m=INS.match(line)
        if not m: continue
        addr=int(m.group(1),16); bs=bytes.fromhex(m.group(2)); mn=m.group(3).lower(); op=m.group(4).strip()
        ins.append({'addr':addr,'bytes':bs.hex(),'len':len(bs),'mn':mn,'op':op})
    return ins

def target(op):
    # objdump direct target is normally first hex token, optionally with annotation
    m=re.search(r'\b0x([0-9a-f]{5,8})\b',op,re.I)
    if m:return int(m.group(1),16)
    m=re.match(r'^([0-9a-f]{5,8})\b',op,re.I)
    return int(m.group(1),16) if m else None

def pe_sections(exe):
    b=Path(exe).read_bytes(); pe=struct.unpack_from('<I',b,0x3c)[0]; nsec=struct.unpack_from('<H',b,pe+6)[0]; opt=pe+24; magic=struct.unpack_from('<H',b,opt)[0]
    entry=struct.unpack_from('<I',b,opt+16)[0]; imagebase=struct.unpack_from('<I',b,opt+28)[0]
    size_opt=struct.unpack_from('<H',b,pe+20)[0]; secbase=opt+size_opt
    secs=[]
    for i in range(nsec):
        p=secbase+i*40; name=b[p:p+8].rstrip(b'\0').decode('ascii','replace'); vs,va,rs,raw=struct.unpack_from('<IIII',b,p+8)
        chars=struct.unpack_from('<I',b,p+36)[0]
        secs.append({'name':name,'va':imagebase+va,'rva':va,'vsize':vs,'raw_size':rs,'raw':raw,'chars':chars})
    return {'imagebase':imagebase,'entry':imagebase+entry,'sections':secs}

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--asm',required=True);ap.add_argument('--exe',required=True);ap.add_argument('--out',required=True);a=ap.parse_args()
 ins=parse_asm(a.asm); idx={x['addr']:i for i,x in enumerate(ins)}; byaddr=set(idx)
 pe=pe_sections(a.exe); textsec=next(s for s in pe['sections'] if s['name']=='.text'); lo=textsec['va']; hi=lo+max(textsec['vsize'],textsec['raw_size'])
 ins=[x for x in ins if lo<=x['addr']<hi]; idx={x['addr']:i for i,x in enumerate(ins)}; byaddr=set(idx)
 call_targets=set(); branch_targets=set(); direct_edges=[]
 for x in ins:
  t=target(x['op'])
  if t is None or t not in byaddr: continue
  if x['mn']=='call': call_targets.add(t); kind='call'
  elif x['mn'].startswith('j'): branch_targets.add(t); kind='branch'
  else: continue
  direct_edges.append({'from':hex(x['addr']),'to':hex(t),'kind':kind})
 pro=set()
 for i,x in enumerate(ins):
  if x['mn']=='push' and x['op'].lower() in ('ebp','edi','esi'):
   # strongest classic frame prologue push ebp; mov ebp,esp within next 2 insns
   if i+1<len(ins) and ins[i+1]['addr']==x['addr']+x['len'] and ins[i+1]['mn']=='mov' and ins[i+1]['op'].replace(' ','').lower() in ('ebp,esp','ebp,esp'):
    pro.add(x['addr'])
 # Common hotpatch prologue mov edi,edi; push ebp; mov ebp,esp
 for i,x in enumerate(ins):
  if x['mn']=='mov' and x['op'].replace(' ','').lower()=='edi,edi' and i+2<len(ins):
   if ins[i+1]['mn']=='push' and ins[i+1]['op'].lower()=='ebp' and ins[i+2]['mn']=='mov' and ins[i+2]['op'].replace(' ','').lower()=='ebp,esp': pro.add(x['addr'])
 starts={pe['entry']}|call_targets|pro
 starts={s for s in starts if s in byaddr}
 # Candidate filtering: call targets that land on obvious interior instructions are retained, but score them.
 records=[]
 for s in sorted(starts):
  i=idx[s]; score=0; reasons=[]
  if s in call_targets: score+=3; reasons.append('call-target')
  if s in pro: score+=4; reasons.append('prologue')
  if s==pe['entry']: score+=10; reasons.append('entry')
  # predecessor sanity: a function start should not normally be target of a direct branch from same region, but calls dominate
  records.append({'start':hex(s),'score':score,'reasons':reasons})
 # Create function extents using next start, then trim trailing padding/int3 only.
 starts_sorted=sorted(starts); funcs=[]
 for j,s in enumerate(starts_sorted):
  end=starts_sorted[j+1] if j+1<len(starts_sorted) else hi
  ii=idx[s]; jj=ii
  while jj<len(ins) and ins[jj]['addr']<end: jj+=1
  body=ins[ii:jj]
  # end at first ret followed only by padding before next start, but keep body up to ret
  actual_end=end
  for k,z in enumerate(body):
   if z['mn'] in RET:
    actual_end=z['addr']+z['len']; break
  if actual_end==end and body:
   # retain end boundary; function may have multiple blocks
   actual_end=end
  funcs.append({'start':hex(s),'end':hex(actual_end),'max_end':hex(end),'size_bytes':actual_end-s,'instructions':len([z for z in body if z['addr']<actual_end]),'score':next(r['score'] for r in records if r['start']==hex(s)),'reasons':next(r['reasons'] for r in records if r['start']==hex(s))})
 # CFG basic blocks per function, using direct branch targets and fallthrough after control transfers
 func_by_start={int(f['start'],16):f for f in funcs}
 block_records=[]
 for f in funcs:
  s=int(f['start'],16); e=int(f['max_end'],16); ii=idx[s]; body=[]
  while ii<len(ins) and ins[ii]['addr']<e: body.append(ins[ii]); ii+=1
  bstarts={s}
  for k,x in enumerate(body):
   t=target(x['op'])
   if t is not None and s<=t<e and x['mn'].startswith('j'): bstarts.add(t)
   if x['mn'] in RET or x['mn']=='jmp':
    if k+1<len(body): bstarts.add(body[k+1]['addr'])
  bs=sorted(bstarts); blocks=[]
  for bi,bsaddr in enumerate(bs):
   be=bs[bi+1] if bi+1<len(bs) else e
   blocks.append({'start':hex(bsaddr),'end':hex(be),'instructions':sum(1 for z in body if bsaddr<=z['addr']<be)})
  f['basic_blocks']=blocks; f['basic_block_count']=len(blocks)
 # confidence stats
 high=sum(1 for f in funcs if f['score']>=7); callonly=sum(1 for f in funcs if f['score']==3)
 out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
 report={'pe_entry':hex(pe['entry']),'text_start':hex(lo),'text_end':hex(hi),'instructions_in_text':len(ins),'function_candidates':len(funcs),'high_confidence':high,'call_target_only':callonly,'prologue_starts':len(pro),'direct_call_targets':len(call_targets),'direct_branch_targets':len(branch_targets),'direct_edges':len(direct_edges),'functions':funcs,'edges':direct_edges}
 (out/'functions_recovered.json').write_text(json.dumps(funcs,indent=2)); (out/'cfg_blocks.json').write_text(json.dumps([{'start':f['start'],'basic_blocks':f['basic_blocks']} for f in funcs],indent=2)); (out/'callgraph.json').write_text(json.dumps(direct_edges,indent=2)); (out/'recovery_report.json').write_text(json.dumps(report,indent=2))
 print(json.dumps({k:v for k,v in report.items() if k!='functions' and k!='edges'},indent=2))
if __name__=='__main__':main()
