#!/usr/bin/env python3
import argparse,json,re
from pathlib import Path
pat=re.compile(r'^\s*([0-9a-f]+):\s+',re.I)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--functions',required=True);ap.add_argument('--out',required=True);a=ap.parse_args()
 fs=json.loads(Path(a.functions).read_text()); out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
 (out/'guest_all.h').write_text('#pragma once\n#include <stdint.h>\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
 with (out/'guest_all.h').open('a') as h:
  for f in fs: h.write(f'void guest_{int(f["start"],16):08x}(void);\n')
  h.write('#ifdef __cplusplus\n}\n#endif\n')
 for f in fs:
  va=int(f['start'],16); name=f'guest_{va:08x}'
  (out/f'{name}.c').write_text(f'#include "guest_all.h"\n#include "../runtime/iamp_runtime.h"\nvoid {name}(void) {{ iamp_untranslated(0x{va:08x}u); }}\n')
 print('generated',len(fs),'guest entry stubs')
if __name__=='__main__':main()
