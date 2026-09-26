#!/usr/bin/env python3
import re,json,hashlib,collections,argparse
from pathlib import Path
pat=re.compile(r'^\s*([0-9a-f]+):\s+(.*?)\s+([a-z][a-z0-9.]*)\s*(.*)$',re.I)
branch=re.compile(r'^(?:j[a-z]+|call)\s+0x([0-9a-f]+)$',re.I)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--asm',required=True);ap.add_argument('--out',required=True);ap.add_argument('--exe');a=ap.parse_args()
 text=Path(a.asm).read_text(errors='replace'); hist=collections.Counter(); funcs=[]; labels={}; edges=[]
 current=None
 for line in text.splitlines():
  m=pat.match(line)
  if not m: continue
  addr=int(m.group(1),16); bytes_=m.group(2); mn=m.group(3).lower(); op=m.group(4).strip()
  hist[mn]+=1
  if current is None or addr!=current['last']+current['byte_len']:
   current={'start':addr,'end':addr,'insns':0,'byte_len':0}; funcs.append(current); labels[addr]=len(funcs)-1
  current['last']=addr; current['end']=addr; current['insns']+=1; current['byte_len']=len(bytes_.split())
  bm=branch.match(mn+' '+op)
  if bm:
   tgt=int(bm.group(1),16); edges.append({'from':addr,'to':tgt,'kind':'call' if mn=='call' else 'branch'})
 funcs=[{'start':hex(x['start']),'end':hex(x['end']),'size_instructions':x['insns'],'size_bytes':x['byte_len']} for x in funcs]
 total=sum(hist.values())
 supported=set('mov push pop lea add sub cmp test xor or and inc dec neg not shl shr sar imul idiv div adc sbb cdq jmp je jne jg jge jl jle ja jae jb jbe jp jnp jns js jo jno jc jnc ret leave call nop setl setg sete setne'.split())
 floatset={k for k in hist if k.startswith('f') or k in {'movss','movsd','addss','addsd','subss','subsd','mulss','mulsd','divss','divsd','movaps','movups','addps','subps','mulps','divps','movq','pfmul','pfadd','pfsub','pfacc','femms','emms','punpckhdq','punpckldq','paddd','paddw','psubw','psrad','psrlq','pmaddwd','pand','pxor','andps','xorps','shufps'}}
 known= sum(v for k,v in hist.items() if k in supported)
 report={'instructions':total,'unique_mnemonics':len(hist),'functions_or_regions':len(funcs),'edges':len(edges),'known_integer_control_instructions':known,'known_integer_control_pct':round(100*known/total,2),'floating_simd_instructions':sum(hist[k] for k in floatset),'top_mnemonics':hist.most_common(100),'functions':funcs,'edges':edges}
 out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
 (out/'instruction_histogram.json').write_text(json.dumps(hist,indent=2))
 (out/'functions.json').write_text(json.dumps(funcs,indent=2))
 (out/'edges.json').write_text(json.dumps(edges,indent=2))
 (out/'full_recomp_report.json').write_text(json.dumps(report,indent=2))
 md=f'''# TH075 Full Recompilation Feasibility\n\n- Instructions parsed: **{total:,}**\n- Mnemonics: **{len(hist)}**\n- Linear regions: **{len(funcs):,}**\n- Direct call/branch edges: **{len(edges):,}**\n- Integer/control instructions covered by the initial lifter set: **{known:,} ({100*known/total:.2f}%)**\n- Floating/SIMD instruction occurrences: **{sum(hist[k] for k in floatset):,}**\n\nThis is a feasibility inventory, not a claim that the game is already translated. The remaining work is semantic lifting, indirect control-flow recovery, x87/MMX/SSE state modeling, PE import binding, and the native Vita backends.\n'''
 (out/'REPORT.md').write_text(md)
 print(md)
if __name__=='__main__':main()
