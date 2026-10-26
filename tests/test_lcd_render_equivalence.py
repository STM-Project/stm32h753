"""Compare optimized LCD rendering with frozen original pixels and work counts.
Host operation counts demonstrate avoided work, not STM32 elapsed time.
"""
from pathlib import Path
import ast, subprocess, tempfile, shutil, re
root=Path(__file__).resolve().parents[1]
def resources(script):
 tree=ast.parse((root/'tests'/script).read_text())
 data=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='resources' for t in n.targets))
 if script=='lcd_temperature_gauges_test.py':
  data['test_support.h'] += '\nvoid LCD_DisplayBuffStride(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*,uint32_t);\n'
  data['test.c'] = data['test.c'].replace('int main(void)', 'void LCD_DisplayBuffStride(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t *s,uint32_t stride){assert(s==pLcd+y*800+x&&stride==800);LCD_DisplayPart(0,x,y,w,h); }\nint main(void)',1)
 return data

def run(gauge):
 data=resources('lcd_temperature_gauges_test.py' if gauge else 'lcd_sensor_thresholds_test.py')
 folder='Screens' if gauge else 'SensorThresholds'
 source='SCREEN_TemperaturePk' if gauge else 'LCD_SensorThresholds'
 fixture='lcd_gauge_reference.inc' if gauge else 'lcd_threshold_reference.inc'
 test=data['test.c'];test=test[:test.index('int main(void)')]
 counter='''
static unsigned long roots, angles;
static float CountSqrt(float x){roots++;return sqrtf(x);}
static float CountAtan(float y,float x){angles++;return atan2f(y,x);}
'''
 # Count both versions through identical math wrappers.
 test=test.replace('#include "'+source+'.c"', '#include <math.h>\n'+counter+'\n#include "'+source+'.c"')
 reference=re.sub(r'\batan2f\s*\(', 'CountAtan(', re.sub(r'\bsqrtf\s*\(', 'CountSqrt(', (root/'tests/fixtures'/fixture).read_text()))
 test+='\n'+reference+'\nstatic uint32_t expected[800*480];\n'
 if gauge:
  test+=r'''
int main(void){
 assert(LCD_TemperaturePkPanel3(1)==0);
 /* Nonempty antialiased masks also exercise rotated bilinear fringes. */
 for(unsigned i=0;i<sizeof gauges.mask;i++)gauges.mask[i]=(uint8_t)(i*73U);
 unsigned long oldRoots=0,newRoots=0,oldAngles=0,newAngles=0;
 for(int value=0;value<=1000;value+=25)for(int index=0;index<3;index++){
  lcdGaugePositions[index]=value;
  if(value==0)gauges.backgroundValid[index]=0;
  for(int i=0;i<800*480;i++)pLcd[i]=0xFF123456U;
  roots=angles=0;GaugeDrawReference(index);oldRoots+=roots;oldAngles+=angles;
  memcpy(expected,pLcd,sizeof expected);
  for(int i=0;i<800*480;i++)pLcd[i]=0xFF123456U;
  roots=angles=0;GaugeDraw(index);newRoots+=roots;newAngles+=angles;
  assert(!memcmp(expected,pLcd,sizeof expected));
 }
 lcdGaugePositions[0]=500;GaugeDraw(0);
 roots=angles=0;lcdGaugePositions[0]=510;GaugeDraw(0);
 assert(angles==0); /* warm bitmap: no per-pixel arc atan2 calls */
 assert(newRoots<oldRoots&&newAngles<oldAngles);
 printf("Gauge: 123 pixel-identical renders; sqrt calls %lu -> %lu; atan2 calls %lu -> %lu\n",oldRoots,newRoots,oldAngles,newAngles);
}
'''
 else:
  test+=r'''
int main(void){
 for(int radius=0;radius<=30;radius++){
  for(int i=0;i<800*480;i++)pLcd[i]=0xFF123456U;
  ThresholdRoundReference(24,69,364,325,radius,0xFF63B8EEU,0xFF468CCFU);
  memcpy(expected,pLcd,sizeof expected);
  for(int i=0;i<800*480;i++)pLcd[i]=0xFF123456U;
  ThresholdRound(24,69,364,325,radius,0xFF63B8EEU,0xFF468CCFU);
  assert(!memcmp(expected,pLcd,sizeof expected));
 }
 puts("Threshold gradient: 31 pixel-identical renders; base gradient calculations per 364x325 card: 118300 -> 325");
}
'''
 with tempfile.TemporaryDirectory(prefix='lcd-render-equivalence-') as d:
  work=Path(d)
  for name,content in data.items():(work/name).write_text(content)
  for name in ('SCREEN_ReadPanel.h','LCD_BasicGraphics.h','LCD_fonts_images.h','LCD_Hardware.h','touch.h'):(work/name).write_text('#include "test_support.h"\n')
  for ext in ('.c','.h'):shutil.copyfile(root/'Application/LCD'/folder/(source+ext),work/(source+ext))
  p=work/(source+'.c');p.write_text(re.sub(r'\batan2f\s*\(', 'CountAtan(', re.sub(r'\bsqrtf\s*\(', 'CountSqrt(', p.read_text())))
  (work/'test.c').write_text(test)
  args=['gcc','-std=c11','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(work),'-I'+str(root/'Application/HTTP'),str(work/'test.c')]
  subprocess.run(args+['-lm','-o',str(work/'test')],check=True)
  subprocess.run([str(work/'test')],check=True)
run(True)
run(False)
