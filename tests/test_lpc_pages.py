#!/usr/bin/env python3
"""Check imported SD pages and real LPC settings persistence without hardware."""
import json, subprocess, tempfile, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'Application/LPC2148'
C=r'''
#include "lpc_pages.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *LPC_DefaultFromH7(const char *name){(void)name;return NULL;}
static unsigned char disk[LPC_BLOB_SIZE];static int exists,fail;
static int load(void *out,size_t n,void *ctx){(void)ctx;if(!exists)return 0;memcpy(out,disk,n);return 1;}
static int save(const void *data,size_t n,void *ctx){(void)ctx;if(fail)return -1;assert(LPC_ValidateSettings(data,n));memcpy(disk,data,n);exists=1;return 1;}
static char url[2000000],json[2000000];
static void encode(const char *s,size_t *n){for(;*s;s++)*n+=(size_t)sprintf(url+*n,"%%%02X",(unsigned char)*s);}
int main(void){
 assert(LPC_SettingsInit(load,save,NULL)==0);
 for(unsigned p=1;p<=LPC_PAGE_COUNT;p++){
  char path[40];sprintf(path,"/nowa%u",p);assert(LPC_ResolvePage(path,strlen(path))==p);
  sprintf(path,"/api/lpc%u",p);assert(LPC_ResolveSettingsApi(path,strlen(path))==p);
  assert(LPC_SettingsJson(p,json,sizeof json)>0);
 }
 assert(!LPC_ResolvePage("/nowa01",7));assert(!LPC_ResolvePage("/nowa196",8));
 unsigned tested=0;
 for(unsigned p=1;p<=LPC_PAGE_COUNT && tested<24;p++){
  const LpcPage *page=&LPC_Pages[p-1];
  for(unsigned f=0;f<page->form_count && tested<24;f++){
   const LpcForm *form=&LPC_Forms[page->first_form+f];if(!form->count)continue;
   size_t n=(size_t)sprintf(url,"/lpc.cgi/A117?lpc_page=%u&lpc_form=%u",p,f);
   for(unsigned i=0;i<form->count;i++){
    unsigned id=LPC_Members[form->start+i];const LpcField *field=&LPC_Fields[id];
    if(field->kind==1)continue;
    url[n++]='&';encode(field->name,&n);url[n++]='=';
    char value[257]="";
    if(field->kind==3 && *field->choices){size_t len=strcspn(field->choices,"|");memcpy(value,field->choices,len);value[len]=0;}
    encode(value,&n);
   }
   url[n]=0;unsigned result=0;assert(LPC_ApplyTarget(url,n,&result)==200);assert(result==p);
   assert(LPC_SettingsInit(load,save,NULL)==1);
   for(unsigned i=0;i<form->count;i++){const LpcField *field=&LPC_Fields[LPC_Members[form->start+i]];if(field->kind!=2)assert(LPC_GetSavedSetting(field->name));}
   fail=1;assert(LPC_ApplyTarget(url,n,&result)==503);fail=0;
   strcat(url,"&unknown_field=1");assert(LPC_ApplyTarget(url,strlen(url),&result)==400);tested++;
  }
 }
 assert(tested==24);disk[100]^=1;assert(!LPC_ValidateSettings(disk,sizeof disk));
 puts("PASS: all numbered routes/config APIs; 24 real forms, checkbox clearing, persistence/reload, CRC, unknown fields and failed SD writes.");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(C)
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(SRC),str(p/'test.c'),str(SRC/'lpc_manifest.c'),str(SRC/'lpc_settings.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
spec=json.loads((SRC/'pages.json').read_text())
with zipfile.ZipFile(SRC/'sdcard.zip') as z:
 for page in spec['pages']:
  html=z.read('LPC/P%05d.HTM'%page['number']).decode()
  assert '<%' not in html
  assert len(html.encode())<2*1024*1024
 print('PASS:',len(spec['pages']),'SD pages, inline resources and bounded page sizes.')
# A truncated inline library can pass C/build checks; execute representative documents.
from playwright.sync_api import sync_playwright
with zipfile.ZipFile(SRC/'sdcard.zip') as archive, sync_playwright() as playwright:
 browser=playwright.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for number in [1,2,3,10,50,100,195]:
  page=browser.new_page();errors=[]
  page.on('pageerror',lambda error:errors.append(str(error)))
  def respond(route):
   url=route.request.url
   if '/api/lpcdata' in url:route.fulfill(json={'temperatures':[],'inputs':[],'outputs':[]})
   elif '/api/lpc' in url:route.fulfill(json={'fields':{}})
   else:route.fulfill(body=archive.read('LPC/P%05d.HTM'%number),content_type='text/html; charset=utf-8')
  page.route('**/*',respond)
  page.goto('http://stm32.test/nowa'+str(number));page.wait_for_timeout(200)
  assert page.locator('#lpc-status').count()==1
  assert not errors,(number,errors)
  page.close()
 browser.close()
print('PASS: 7 representative imported pages execute in Chromium.')
