#!/usr/bin/env python3
"""Check real H7 settings card sizes/contrast in Chromium (not LPC pages)."""
import ctypes as C,json,subprocess,tempfile
from pathlib import Path
from playwright.sync_api import sync_playwright
from test_http_settings_page import configure,render
from test_settings_templates import literal
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as name:
 tmp=Path(name);source=(ROOT/'Application/wwwPages.c').read_text()
 (tmp/'common.c').write_text('const char HttpStyle[]='+json.dumps(literal(source,'HttpStyle'))+';\nconst char HttpMainMenu[]='+json.dumps(literal(source,'HttpMainMenu'))+';\nunsigned HTTP_SettingsPortMode(unsigned p){return p%14;}')
 files=[str(ROOT/'Application/HTTP'/n) for n in ('http_settings_catalog.c','http_settings_model.c','http_settings_page.c')]
 subprocess.run(['gcc','-shared','-fPIC','-I'+str(ROOT/'Application/HTTP'),str(tmp/'common.c'),*files,'-o',str(tmp/'page.so')],check=True)
 lib=C.CDLL(str(tmp/'page.so'));configure(lib);lib.HTTP_SettingsModelInit(None,None,None)
 with sync_playwright() as pw:
  browser=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  page=browser.new_page(viewport={'width':1440,'height':1000})
  total=0
  for i in range(lib.HTTP_SettingsCatalogCount()):
   schema=lib.HTTP_SettingsCatalogAt(i);route=schema.contents.route.decode()
   if route=='/set':continue
   page.set_content(render(lib,schema,1 if schema.contents.max_instance else 0),wait_until='domcontentloaded')
   cards=page.locator('.corner-radius,.corner-radius3:not(.port-column)')
   metrics=cards.evaluate_all("es=>es.map(e=>({height:e.offsetHeight,scroll:e.scrollHeight,client:e.clientHeight,font:getComputedStyle(e).fontSize,color:getComputedStyle(e).color}))")
   for m in metrics:
    assert m['scroll']<=m['client']+2,(route,'vertical overflow',m)
    assert int(float(m['font'][:-2]))<=20,(route,'oversized text',m)
   for card in cards.all():
    rows=card.locator(':scope > .h7-field-row')
    if rows.count():
     starts=rows.locator('.h7-field-controls').evaluate_all('es=>es.map(e=>e.getBoundingClientRect().left)')
     assert max(starts)-min(starts)<2,(route,'unaligned fields',starts)
     positions=rows.evaluate_all('es=>es.map(e=>{let r=e.getBoundingClientRect();return [r.top,r.bottom]})')
     for before,after in zip(positions,positions[1:]):
      assert after[0]-before[1]>=9,(route,'field rows too close')
    assert card.evaluate('e=>parseFloat(getComputedStyle(e).paddingBottom)')>=22,(route,'bottom padding')
   if route=='/lan':page.screenshot(path='/tmp/h7-lan-fields.png',full_page=True)
   save=page.locator('.corner-radius').filter(has_text='Zapis ustawien')
   if save.count():assert save.last.bounding_box()['height']<220,(route,'oversized save card')
   if route=='/Rot':
    table=page.locator('.corner-radius table').first
    assert table.evaluate('e=>getComputedStyle(e).color')=='rgb(238, 238, 238)'
    assert table.evaluate('e=>parseFloat(getComputedStyle(e).fontSize)')<=16
    assert cards.first.bounding_box()['height']<1300
    page.screenshot(path='/tmp/h7-rotation.png',full_page=True)
   page.set_viewport_size({'width':390,'height':844})
   assert page.locator('button[type=submit]').count()==0 or page.locator('button[type=submit]').first.is_visible()
   total+=1
   page.set_viewport_size({'width':1440,'height':1000})
  browser.close()
 print('PASS:',total,'H7 settings types; content-sized cards, compact save cards, readable rotation table and mobile controls.')

# Exercise the real /main and /set assembly, including port-column CSS overrides.
from test_http_sensor_routes import harness_source,compile_and_run,TESTS
with tempfile.TemporaryDirectory() as directory:
 main_path=Path(directory)/'main.html';set_path=Path(directory)/'set.html'
 fixture=TESTS.replace('int main(void)','int original_tests(void)')+r'''
int main(void){
 HTTP_SettingsModelInit(NULL,NULL,NULL);HTTP_SensorSettingsInit(NULL,NULL,NULL);
 static char html[250000];
 request(0,"/main");body(0,html,sizeof html);FILE*f=fopen(MAIN_FILE,"w");assert(f);fputs(html,f);fclose(f);
 request(0,"/set");body(0,html,sizeof html);f=fopen(SET_FILE,"w");assert(f);fputs(html,f);fclose(f);
 return 0;
}
'''
 fixture=fixture.replace('MAIN_FILE',json.dumps(str(main_path))).replace('SET_FILE',json.dumps(str(set_path)))
 compile_and_run(harness_source(fixture))
 with sync_playwright() as pw:
  browser=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  for path in (main_path,set_path):
   page=browser.new_page(viewport={'width':1440,'height':1000})
   page.route('**/*',lambda route:route.fulfill(body=path.read_text(),content_type='text/html; charset=utf-8'))
   page.goto('http://stm32.test/'+path.stem)
   if path==main_path:
    boxes=[page.locator('.h7-read-grid>.y').nth(i).bounding_box() for i in range(5)]
    assert max(b['y'] for b in boxes[:4])-min(b['y'] for b in boxes[:4])<2
    assert boxes[4]['y']>=boxes[0]['y']+boxes[0]['height']
    assert boxes[0]['height']<180
   else:
    boxes=[page.locator('.port-column').nth(i).bounding_box() for i in range(3)]
    assert max(b['y'] for b in boxes)-min(b['y'] for b in boxes)<2
   page.close()
  browser.close()
 print('PASS: /main compact aligned rows and /set aligned port columns.')
