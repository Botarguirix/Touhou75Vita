#!/usr/bin/env python3
"""Lift objdump x86 text into a compact, deterministic IR.
This is a real semantic-lifting stage: instructions become typed IR records,
not untranslated entry stubs. Unsupported mnemonics are explicitly marked.
"""
import argparse,json,re
from pathlib import Path
INS=re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s+([a-z][a-z0-9.]*)\s*(.*)$',re.I)
REGS={'eax','ebx','ecx','edx','esi','edi','ebp','esp','ax','bx','cx','dx','si','di','bp','sp','al','bl','cl','dl','ah','bh','ch','dh'}
JCC={'jo','jno','jb','jae','je','jz','jne','jnz','jbe','ja','jl','jge','jle','jg','js','jns','jp','jnp','jc','jnc'}
ARITH={'add','adc','sub','sbb','and','or','xor','cmp','test','inc','dec','neg','not','imul','mul','idiv','div','shl','shr','sar','rol','ror','rcl','rcr'}
X87_PREFIX=('f','fucom','fcom','fld','fst','fx','fn','fadd','fsub','fmul','fdiv','fchs','fabs','fsqrt','fsin','fcos','fptan','fpatan','f2xm1','fyl2x')
SIMD_PREFIX=('movd','movq','movaps','movups','movss','movsd','addps','subps','mulps','divps','addss','subss','mulss','divss','andps','orps','xorps','shufps','unpck','punpck','padd','psub','pmul','pmadd','psrl','psra','psll','pack','pcmpeq','pcmpgt','emms','femms')

def parse(path):
 out=[]
 for line in Path(path).read_text(errors='replace').splitlines():
  m=INS.match(line)
  if m:
   bs=bytes.fromhex(m.group(2)); out.append((int(m.group(1),16),bs,m.group(3).lower(),m.group(4).strip()))
 return out

def classify(mn):
 if mn in JCC or mn in {'jmp','call','ret','retn','retf','iret','leave','loop','loope','loopne'}: return 'control'
 if mn in {'mov','movzx','movsx','lea','push','pop','xchg','bswap','cdq','cwd','cwde','cbw'}: return 'data'
 if mn in ARITH: return 'integer'
 if mn.startswith('f') or mn.startswith('fu') or mn.startswith('fx'): return 'x87'
 if mn.startswith(SIMD_PREFIX): return 'simd'
 if mn.startswith('set') or mn.startswith('cmov'): return 'flags'
 if mn in {'nop','int3','ud2','hlt','clc','stc','cli','sti','cld','std'}: return 'misc'
 return 'unsupported'

def reg_width(r):
 r=r.lower()
 if r in REGS:
  return 8 if len(r)==2 and r[1] in 'lh' else 16 if len(r)==2 else 32
 return None

def operands(op):
 # split commas outside brackets
 out=[]; cur=''; depth=0
 for c in op:
  if c=='[': depth+=1
  elif c==']': depth-=1
  if c==',' and depth==0: out.append(cur.strip()); cur=''
  else: cur+=c
 if cur.strip(): out.append(cur.strip())
 return out

def norm_operand(x):
 x=x.strip()
 xl=x.lower().replace('ptr','').strip()
 if xl.startswith('ds:'): xl=xl[3:]
 if re.fullmatch(r'(e?[abcd]x|e?[sd]i|e?[sb]p|e?bp|[abcd][lh])',xl): return {'kind':'reg','name':xl,'width':reg_width(xl)}
 m=re.search(r'0x([0-9a-f]+)$',xl)
 if m and '[' not in xl: return {'kind':'imm','value':int(m.group(1),16)}
 m=re.fullmatch(r'([-+]?\d+)',xl)
 if m: return {'kind':'imm','value':int(m.group(1),10)}
 if '[' in xl or ']' in xl: return {'kind':'mem','expr':xl}
 return {'kind':'text','text':x}

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--asm',required=True);ap.add_argument('--out',required=True);ap.add_argument('--limit',type=int,default=0);a=ap.parse_args()
 ins=parse(a.asm); limit=a.limit or len(ins); ins=ins[:limit]
 rows=[]; counts={}; unsupported=[]
 for addr,bs,mn,op in ins:
  cat=classify(mn); counts[mn]=counts.get(mn,0)+1
  row={'addr':hex(addr),'len':len(bs),'mn':mn,'cat':cat,'operands':[norm_operand(x) for x in operands(op)]}
  if cat=='unsupported': unsupported.append(mn)
  rows.append(row)
 out=Path(a.out);out.parent.mkdir(parents=True,exist_ok=True)
 # JSONL keeps the complete IR stream scalable.
 with out.open('w') as f:
  for r in rows: f.write(json.dumps(r,separators=(',',':'))+'\n')
 summary={'instructions':len(rows),'categories':{},'unsupported_mnemonics':sorted(set(unsupported)),'unsupported_count':sum(1 for r in rows if r['cat']=='unsupported'),'mnemonics':sorted(counts)}
 for r in rows: summary['categories'][r['cat']]=summary['categories'].get(r['cat'],0)+1
 out.with_suffix('.summary.json').write_text(json.dumps(summary,indent=2))
 print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
