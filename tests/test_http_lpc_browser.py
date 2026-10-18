#!/usr/bin/env python3
"""Optional Chromium/Playwright test of the offline LPC panel and real C data model."""
from pathlib import Path
import subprocess,ctypes as c,json,tempfile,sys
work=tempfile.TemporaryDirectory(prefix="lpc-browser-")
library=str(Path(work.name)/"live.so")
from urllib.parse import urlsplit,urlencode
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1];app=root/'Application'
sys.path.insert(0,str(root/'tools'))
from generate_lpc_assets import build_html
html=build_html()
subprocess.run(['gcc','-shared','-fPIC','-std=c11','-I'+str(app),'-I'+str(app/'HTTP'),*[str(app/n) for n in ['HTTP/http_lpc_panel.c','HTTP/http_sensor_settings.c','HTTP/http_settings_model.c','HTTP/http_settings_catalog.c','relay_states.c']],'-o',library],check=True)
l=c.CDLL(library);l.HTTP_LpcPanelJsonFromRefresh.argtypes=[c.c_char_p,c.c_size_t,c.c_char_p,c.c_size_t];l.HTTP_LpcPanelJsonFromRefresh.restype=c.c_size_t
refresh=c.create_string_buffer(b" "*120)
save_sensor=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_size_t,c.c_void_p)(lambda *_:1)
l.HTTP_SettingsModelInit(None,None,None);l.HTTP_SensorSettingsInit(None,save_sensor,None);l.RELAY_ResetStates();l.HTTP_LpcResetMeasurements()
def data():
 b=c.create_string_buffer(20000);assert l.HTTP_LpcPanelJsonFromRefresh(b,len(b),refresh,120);return b.value.decode()
with sync_playwright() as p:
 browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox'])
 page=browser.new_page(viewport={'width':1500,'height':1100});errors=[];requests=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 def respond(route):
  url=urlsplit(route.request.url);requests.append(url.path)
  assert url.hostname=='stm32.test',url
  if url.path=='/mobile/pk001':
   actual=l.HTTP_LpcOutputStateFromRefresh(refresh,120,28)
   if actual>=0:assert l.RELAY_UpdateState(28,actual)
   assert l.RELAY_RequestToggle(28);route.fulfill(content_type='text/plain',body='OK');return
  if url.path=='/api/nowa':route.fulfill(content_type='application/json',body=data());return
  assert url.path=='/nowa',('Unexpected resource request',url.path)
  route.fulfill(content_type='text/html; charset=UTF-8',body=html)
 page.route('**/*',respond)
 page.goto('http://stm32.test/nowa',wait_until='networkidle')
 page.wait_for_function("document.querySelector('#connection').textContent.includes('Połączenie OK')")
 assert page.locator('#tu_temp001').inner_text()=='brak danych'
 for channel in range(1,5):
  assert not page.locator('[data-output="'+str(channel)+'"]').is_disabled()
 for offset,value in [(73,b' 25.1'),(85,b'-12.3'),(97,b' 40.5')]:refresh[offset:offset+5]=value
 refresh[42:44]=b'01';refresh[25:29]=b'1000'
 page.wait_for_function("document.querySelector('#tu_temp001').textContent==='25.1'")
 page.wait_for_timeout(1300)
 assert page.locator('#tu_temp002').inner_text()=='-12.3'
 assert page.locator('#tu_temp003').inner_text()=='40.5'
 assert page.locator('#tu_we001').inner_text()=='ON'
 assert page.locator('#tu_we002').inner_text()=='OFF'
 assert 'ON' in page.locator('#tu_pk001').inner_text()
 page.locator('[data-output="1"]').click()
 page.wait_for_function("document.querySelector('#tu_pk001').textContent.includes('zadano OFF')")
 assert page.locator('#tu_pk001').inner_text().startswith('ON')
 # Change persisted scale, correction and escaped name while the page is open.
 values={'nazwa':'Salon "A" & <B>','prog_max1':'60','prog_max2':'70','prog_min1':'-20','prog_min2':'-30','hist':'0.5','korekcja':'-1.2','czas_reak':'00:00'}
 fields={'n_s_Czujki_'+key+'_001':value for key,value in values.items()};fields['n_109_zapisz']=''
 target=('/lpc.cgi/A117?'+urlencode(fields)).encode();port=c.c_uint()
 assert l.HTTP_SensorSettingsApplyTarget(target,len(target),c.byref(port))==1
 page.wait_for_function("document.querySelector('#tu_temp001').textContent==='25.1'")
 page.wait_for_function("document.querySelector('#fixture').textContent.includes('60')")
 assert page.locator('#name-temp-1').inner_text()=='Salon "A" & <B> (port 1)'
 assert page.locator('#name-temp-1 b').count()==0
 assert page.locator('.temperatures svg').count()==3
 assert page.locator('#container svg').count()==1
 assert not errors,errors
 assert requests.count('/nowa')==1 and set(requests)<={'/nowa','/api/nowa','/mobile/pk001'},requests
 assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
 page.screenshot(path=str(Path(work.name)/"desktop.png"),full_page=True)
 page.set_viewport_size({'width':390,'height':844});page.wait_for_timeout(600)
 assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
 page.screenshot(path=str(Path(work.name)/"mobile.png"),full_page=True)
 refresh[73:78]=b' --.-'
 page.wait_for_function("document.querySelector('#tu_temp001').textContent==='brak danych'")
 assert page.locator('#fixture').is_visible()
 assert 'no-measurement' in page.locator('#fixture').get_attribute('class')
 print('PASS: offline Chromium panel; three actual SVG thermometers, live chart, measurements/inputs/relay feedback, missing sensor, output command separated from feedback and desktop/mobile layout. Requests:',len(requests))
 browser.close()

work.cleanup()
