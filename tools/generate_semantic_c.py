#!/usr/bin/env python3
import argparse,json,re
from pathlib import Path

INS_RE=re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s+([a-z][a-z0-9.]*)\s*(.*)$',re.I)
JCC={'jo','jno','jb','jae','je','jz','jne','jnz','jbe','ja','jl','jge','jle','jg','js','jns','jp','jnp','jc','jnc','jcxz','jecxz'}
RET={'ret','retn','retf','iret'}
DATA={'mov','movzx','movsx','lea','push','pop','xchg','bswap','cdq','cwd','cwde','cbw'}
ARITH={'add','adc','sub','sbb','and','or','xor','cmp','test','inc','dec','neg','not','imul','mul','idiv','div','shl','shr','sar','rol','ror','rcl','rcr'}
FLAGS={'seto','setno','setb','setae','sete','setz','setne','setnz','setbe','seta','setl','setge','setle','setg','sets','setns','setp','setnp','setc','setnc'}


def parse_asm(path):
 out=[]
 for line in Path(path).read_text(errors='replace').splitlines():
  m=INS_RE.match(line)
  if m:
   out.append({'addr':int(m.group(1),16),'bytes':m.group(2).strip(),'mn':m.group(3).lower(),'op':m.group(4).strip()})
 return out

def splitops(op):
 out=[]; cur=''; dep=0
 for c in op:
  if c=='[': dep+=1
  elif c==']': dep-=1
  if c==',' and dep==0: out.append(cur.strip()); cur=''
  else: cur+=c
 if cur.strip(): out.append(cur.strip())
 return out

def target(op):
 m=re.search(r'\b0x([0-9a-f]{5,8})\b',op,re.I)
 if m:return int(m.group(1),16)
 m=re.match(r'^([0-9a-f]{5,8})\b',op,re.I)
 return int(m.group(1),16) if m else None

def caddr(a): return f'guest_{a:08x}'

def esc(s): return s.replace('\\','\\\\').replace('"','\\"')

def main():
 ap=argparse.ArgumentParser(); ap.add_argument('--asm',required=True); ap.add_argument('--functions',required=True); ap.add_argument('--out',required=True); a=ap.parse_args()
 ins=parse_asm(a.asm); by={x['addr']:x for x in ins}; funcs=json.load(open(a.functions))
 fstarts={int(f['start'],16) for f in funcs}
 # only emit functions whose start exists in disassembly
 funcs=[f for f in funcs if int(f['start'],16) in by]
 p=Path(a.out); p.parent.mkdir(parents=True,exist_ok=True)
 with p.open('w') as o:
  o.write('''#include "iamp_semantic_runtime.h"\n#include <stdint.h>\n#include <stddef.h>\n\n''')
  for f in funcs: o.write(f'void {caddr(int(f["start"],16))}(IAMP_CPU *s);\n')
  o.write('\nvoid iamp_dispatch(IAMP_CPU*, uint32_t);\n\n')
  unsupported={}
  emitted=0
  for f in funcs:
   s=int(f['start'],16); e=int(f['max_end'],16)
   body=[]; cur=s
   while cur<e and cur in by:
    body.append(by[cur]); cur += 1 # fixed below using length
    body[-1]['next']=body[-1]['addr']+len(bytes.fromhex(body[-1]['bytes']))
    cur=body[-1]['next']
   # boundaries for labels: all direct branch targets inside function
   body_addrs={x['addr'] for x in body}
   labels={s}
   for x in body:
    t=target(x['op'])
    if t is not None and t in body_addrs and (x['mn'].startswith('j') or x['mn'] in ('call',)):
     labels.add(t)
   o.write(f'void {caddr(s)}(IAMP_CPU *s){{\n')
   o.write(f'  /* 0x{s:08x} */\n')
   for x in body:
    if x['addr'] in labels: o.write(f'L_{x["addr"]:08x}:;\n')
    mn=x['mn']; ops=splitops(x['op']); t=target(x['op'])
    if mn in ('nop','int3'):
     if mn=='int3': o.write(f'  iamp_trap(s,0x{x["addr"]:08x},"int3");\n')
    elif mn in DATA:
     if mn=='push': o.write(f'  iamp_push(s, iamp_readop(s,"{esc(ops[0])}",4));\n')
     elif mn=='pop': o.write(f'  iamp_writeop(s,"{esc(ops[0])}",4,iamp_pop(s));\n')
     elif mn=='lea': o.write(f'  iamp_lea(s,"{esc(ops[0])}","{esc(ops[1])}");\n')
     elif len(ops)>=2: o.write(f'  iamp_{mn}(s,"{esc(ops[0])}","{esc(ops[1])}");\n')
     else: o.write(f'  iamp_trap(s,0x{x["addr"]:08x},"{mn}");\n')
    elif mn in ARITH:
     if len(ops)>=2: o.write(f'  iamp_{mn}(s,"{esc(ops[0])}","{esc(ops[1])}");\n')
     elif len(ops)==1: o.write(f'  iamp_{mn}(s,"{esc(ops[0])}",NULL);\n')
     else:o.write(f'  iamp_trap(s,0x{x["addr"]:08x},"{mn}");\n')
    elif mn in FLAGS:
     o.write(f'  iamp_{mn}(s,"{esc(ops[0])}");\n')
    elif mn in JCC:
     if t is not None and t in body_addrs: o.write(f'  if(iamp_cond(s,"{mn}")) goto L_{t:08x};\n')
     elif t is not None: o.write(f'  if(iamp_cond(s,"{mn}")) iamp_jump(s,0x{t:08x});\n')
     else:o.write(f'  iamp_trap(s,0x{x["addr"]:08x},"{mn}");\n')
    elif mn=='jmp':
     if t is not None and t in body_addrs: o.write(f'  goto L_{t:08x};\n')
     elif t is not None:o.write(f'  iamp_jump(s,0x{t:08x}); return;\n')
     else:o.write(f'  iamp_indirect_jump(s,"{esc(ops[0])}"); return;\n')
    elif mn=='call':
     if t is not None: o.write(f'  iamp_call(s,0x{t:08x},0x{x["next"]:08x});\n')
     else:o.write(f'  iamp_indirect_call(s,"{esc(ops[0])}",0x{x["next"]:08x});\n')
    elif mn in RET:
     o.write('  iamp_ret(s); return;\n')
    elif mn=='leave': o.write('  iamp_leave(s);\n')
    elif mn in ('pusha','pushad'): o.write('  iamp_pusha(s);\n')
    elif mn in ('popa','popad'): o.write('  iamp_popa(s);\n')
    elif mn=='pushf' or mn=='pushfd': o.write('  iamp_pushf(s);\n')
    elif mn=='popf' or mn=='popfd': o.write('  iamp_popf(s);\n')
    elif mn=='lahf': o.write('  iamp_lahf(s);\n')
    elif mn=='cpuid': o.write('  iamp_cpuid(s);\n')
    elif mn=='xlat': o.write('  iamp_xlat(s);\n')
    elif mn in ('bt','bts','btr','btc'): o.write(f'  iamp_bit(s,"{esc(mn)}","{esc(x["op"])}");\n')
    elif mn in ('shld','shrd'): o.write(f'  iamp_double_shift(s,"{esc(mn)}","{esc(x["op"])}");\n')
    elif mn.startswith('cmov'): o.write(f'  iamp_cmov(s,"{esc(mn)}","{esc(x["op"])}");\n')
    elif mn=='sahf': o.write('  iamp_sahf(s);\n')
    elif mn in ('clc','stc','cld','std','cdq','cwd','cwde','cbw','emms'):
     o.write(f'  iamp_{mn}(s);\n')
    elif mn.startswith('f'):
     o.write(f'  iamp_x87(s,"{esc(mn)}","{esc(ops[0]) if ops else ""}","{esc(ops[1]) if len(ops)>1 else ""}");\n')
    elif mn in ('rep','repz','repnz'):
     # objdump emits the REP prefix as the mnemonic and the actual string op in operands.
     o.write(f'  iamp_string(s,"{esc(mn)}","{esc(x["op"])}");\n')
    elif mn in ('movs','stos','lods','scas','cmps'):
     o.write(f'  iamp_string(s,"","{esc(x["op"])}");\n')
    elif mn in ('movq','movd','movaps','movups','movapd','movlpd','movhpd','movss','movsd','addps','subps','mulps','divps','addss','subss','mulss','divss','andps','orps','xorps','andnps','shufps','movhps','movlps','movhlps','movmskps','sqrtps','rcpps','rsqrtss','unpckhps','unpcklps','unpckhpd','unpcklpd','punpcklbw','punpcklwd','punpckldq','punpckhbw','punpckhwd','punpckhdq','paddb','paddw','paddd','psubb','psubw','psubd','pmullw','pmaddwd','psrlw','psrld','psrlq','psraw','psrad','psllw','pslld','psllq','packssdw','packuswb','pxor','pand','pandn','por','pcmpeqb','pcmpeqw','pcmpeqd','pcmpgtb','pcmpgtw','pcmpgtd','pmulhw','pavgb','pavgw') or mn.startswith(('pf','pi2','pf2','pswap')):
     o.write(f'  iamp_simd(s,"{esc(mn)}","{esc(x["op"])}");\n')
    elif mn in ('emms','sfence','lfence','mfence','ldmxcsr','stmxcsr'):
     o.write(f'  iamp_simd(s,"{esc(mn)}","{esc(x["op"])}");\n')
    else:
     o.write(f'  iamp_unimplemented(s,0x{x["addr"]:08x},"{esc(mn)}");\n'); unsupported[mn]=unsupported.get(mn,0)+1
    emitted+=1
   o.write('}\n\n')
  o.write('void iamp_dispatch(IAMP_CPU*s,uint32_t a){ switch(a){\n')
  for f in funcs: o.write(f'  case 0x{int(f["start"],16):08x}: {caddr(int(f["start"],16))}(s); return;\n')
  o.write('  default: iamp_trap(s,a,"missing translated function");\n }}\n')
 summary={'functions':len(funcs),'instructions_emitted':emitted,'currently_deferred_mnemonics':dict(sorted(unsupported.items(),key=lambda kv:(-kv[1],kv[0])))}
 (p.parent/'semantic_codegen_summary.json').write_text(json.dumps(summary,indent=2))
 print(json.dumps(summary,indent=2))
if __name__=='__main__': main()
