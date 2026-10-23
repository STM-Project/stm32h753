import re, pathlib, xml.etree.ElementTree as E, subprocess, concurrent.futures, json, os, shutil
root=pathlib.Path(__file__).resolve().parents[1]; out=root/'Debug/cloud'; out.mkdir(parents=True,exist_ok=True)
config=E.parse(root/'.cproject').getroot().find('.//configuration')
opts=config.findall('.//option')
incs=[]; defs=[]
for opt in opts:
 s=opt.get('superClass','')
 if '.c.compiler.' in s:
  if opt.get('valueType')=='includePath': incs=['-I'+str((root/'Debug'/x.get('value')).resolve()) for x in opt]
  if opt.get('valueType')=='definedSymbols': defs=['-D'+x.get('value') for x in opt]
overlay=out/'include'; overlay.mkdir(parents=True,exist_ok=True)
headers={}
for p in root.rglob('*.h'): headers.setdefault(p.name.lower(),[]).append(p)
for p in root.rglob('*'):
 if p.suffix in ('.c','.h'):
  for name in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]',p.read_text(errors='replace')):
   matches=headers.get(name.lower(),[])
   if '/' not in name and len(matches)==1 and name!=matches[0].name:
    dest=overlay/name
    if not dest.exists(): dest.symlink_to(matches[0])
incs.insert(0,'-I'+str(overlay))
gcc=os.environ.get('ARM_GCC') or shutil.which('arm-none-eabi-gcc')
if not gcc: raise SystemExit('Set ARM_GCC to arm-none-eabi-gcc or add it to PATH')
flags=['-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-O0','-g3','-ffunction-sections','-fdata-sections','-Wall','-std=gnu11','-Wno-error=implicit-function-declaration','-Wno-error=int-conversion','-Wno-error=incompatible-pointer-types']
sources=[]
for entry in config.findall('.//sourceEntries/entry'):
 sources += sorted(p for p in (root/entry.get('name')).rglob('*') if p.suffix in ('.c','.s','.S'))
def compile(p):
 obj=out/(str(p.relative_to(root))+'.o'); obj.parent.mkdir(parents=True,exist_ok=True)
 cmd=[gcc,*flags,*incs,*defs,'-c',str(p),'-o',str(obj)]
 result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 return obj,result,cmd,p
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: results=list(pool.map(compile,sources))
errors=[]
for obj,r,cmd,p in results:
 if r.stdout: print(str(p.relative_to(root))+':\n'+r.stdout)
 if r.returncode: errors.append(str(p.relative_to(root)))
(out/'compile_commands.json').write_text(json.dumps([{'directory':str(root),'arguments':cmd,'file':str(p)} for _,_,cmd,p in results],indent=2))
print(f'Compiled {len(sources)-len(errors)}/{len(sources)} sources; failures: {errors}')
if errors: raise SystemExit(1)
subprocess.run([gcc,*flags,*map(str,[x[0] for x in results]),'-T'+str(root/'STM32H753BITX_FLASH.ld'),'--specs=nano.specs','--specs=nosys.specs','-Wl,--gc-sections','-Wl,-Map='+str(out/'firmware.map'),'-o',str(out/'firmware.elf'),'-Wl,--start-group','-lc','-lm','-Wl,--end-group'],check=True)
size=pathlib.Path(gcc).with_name('arm-none-eabi-size')
if size.exists(): subprocess.run([str(size),str(out/'firmware.elf')],check=True)
