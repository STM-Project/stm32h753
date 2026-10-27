"""Execute the actual owner-task bridge: coherent LCD snapshots and PWM/relay commands."""
from pathlib import Path
import re,subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1]
mode=re.search(r'typedef enum\s*\{\s*HTTP_MODE_RELAY.*?\} HTTP_PORT_MODE;',(ROOT/'Application/wwwPages.h').read_text(),re.S)[0]
C=r'''
#include "http_lcd_panels.c"
#include <assert.h>
#include <stdlib.h>
static unsigned modes[33],wake,critical;
static int fail_save, committed;
static unsigned char disk[HTTP_SETTINGS_BLOB_SIZE];
static int load(void*b,size_t n,void*c){(void)c;memcpy(b,disk,n);return 1;}
static int save(const void*b,size_t n,void*c){(void)c;assert(!critical);if(fail_save)return -1;memcpy(disk,b,n);return 1;}
void HTTP_SettingsBindingsCommitted(const HTTP_SettingPage*p,uint16_t instance){
 assert(!critical&&instance==0);committed++;
 for(unsigned i=0;i<33;i++){char name[32];snprintf(name,sizeof name,"n_GPIO_val_%03u",i+1);modes[i]=(unsigned)atoi(HTTP_SettingsGetByName(p,0,name));}
}
static char names[33][31];
void enter(void){critical++;} void leave(void){assert(critical);critical--;}
void HTTP_SensorLCDWake(void){wake++;}
unsigned HTTP_SettingsPortMode(unsigned p){assert(!critical&&p>=1&&p<=33);return modes[p-1];}
const char *HTTP_SettingsPortName(unsigned p,const char*f){(void)f;assert(!critical&&p>=1&&p<=33);return names[p-1];}
int main(void){
 HTTP_LCDPanelData out;assert(!HTTP_LCDPanelRead(&out));assert(!HTTP_LCDPanelRead(NULL));
 for(unsigned i=0;i<33;i++){modes[i]=HTTP_MODE_DISABLED;snprintf(names[i],31,"Port %u",i+1);}
 modes[1]=HTTP_MODE_PT1000;modes[0]=HTTP_MODE_TEMP;modes[4]=HTTP_MODE_TEMP_RHU;modes[14]=HTTP_MODE_ADC;modes[16]=HTTP_MODE_COUNTER;
 modes[3]=HTTP_MODE_INPUT;modes[5]=HTTP_MODE_EOL;modes[20]=HTTP_MODE_PWM;modes[27]=HTTP_MODE_RELAY;
 char data[1000];memset(data,' ',sizeof data);memcpy(data,"12:34:56",8);
 memcpy(data+73,"-12.3",5);data[78]='1';memcpy(data+73+48," 25.1",5);memcpy(data+73+48+7,"650",3);
 memcpy(data+321,"  -25.125",9);data[330]='2';memcpy(data+452,"Volt ",5);
 data[39+3]='0';data[58+5]='5';memcpy(data+290,"   5.125 ",9);
 memcpy(data+465,"000000000000123.456Wh",21);memcpy(data+465+23,"00000000012.34",14);
 RELAY_ResetStates();HTTP_LCDPanelActive(1);assert(wake==1&&HTTP_LCDPanelWanted());
 HTTP_LCDPanelService(data,sizeof data,0);assert(HTTP_LCDPanelRead(&out));
 assert(!strcmp(out.time,"12:34:56")&&!strcmp(out.ports[0].value[0],"-12.3 C"));
 assert(out.ports[0].color[0]==ALARM&&!strcmp(out.ports[4].value[1],"65.0 %"));
 assert(!strcmp(out.ports[3].value[0],"Stan ON")&&!strcmp(out.ports[5].value[0],"Uszkodzona linia"));
 assert(!strcmp(out.ports[14].value[0],"5.125 Volt"));
 assert(!strcmp(out.ports[1].value[0],"-25.125 C")&&out.ports[1].color[0]==COLD);
 assert(!strcmp(out.ports[16].value[0],"123.456Wh"));
 uint32_t revision=out.revision;HTTP_LCDPanelService(data,sizeof data,250);HTTP_LCDPanelRead(&out);assert(out.revision==revision);
 RELAY_RequestPwm(21,35);HTTP_LCDPanelService(data,sizeof data,500);HTTP_LCDPanelRead(&out);assert(out.ports[20].pwm==35);
 assert(HTTP_LCDPanelPwm(21,1)&&HTTP_LCDPanelPwm(21,50)&&HTTP_LCDPanelPwm(21,100));
 assert(!HTTP_LCDPanelPwm(20,10)&&!HTTP_LCDPanelPwm(24,10)&&!HTTP_LCDPanelPwm(21,101));
 HTTP_LCDPanelService(data,sizeof data,501);HTTP_LCDPanelRead(&out);assert(out.ports[20].pwm==100&&RELAY_GetState(21)->pwm_percent==100);
 assert(HTTP_LCDPanelPwm(22,80));HTTP_LCDPanelService(data,sizeof data,502);assert(!RELAY_GetState(22)->pwm_valid);
 data[25]='1';assert(HTTP_LCDPanelToggle(28));HTTP_LCDPanelService(data,sizeof data,503);assert(RELAY_GetDisplayState(28)==0);
 assert(HTTP_LCDPanelToggle(28)&&HTTP_LCDPanelToggle(28));HTTP_LCDPanelService(data,sizeof data,504);assert(RELAY_GetDisplayState(28)==0);
 RELAY_RequestToggle(28);HTTP_LCDPanelService(data,sizeof data,800);HTTP_LCDPanelRead(&out);assert(!strcmp(out.ports[27].value[0],"Stan ON"));
 snprintf(names[0],31,"Zmiana z WWW");HTTP_LCDPanelService(NULL,0,1100);HTTP_LCDPanelRead(&out);
 assert(!strcmp(out.ports[0].name,"Zmiana z WWW")&&!strcmp(out.ports[0].value[0],"--- C"));
 HTTP_LCDPanelActive(0);assert(!HTTP_LCDPanelWanted());revision=out.revision;HTTP_LCDPanelService(data,sizeof data,1500);HTTP_LCDPanelRead(&out);assert(out.revision==revision);
 assert(HTTP_LCDPanelPwm(21,0));HTTP_LCDPanelService(data,sizeof data,1501);assert(RELAY_GetState(21)->pwm_percent==0);
 /* Real HTTP schema/model and save callback, including transaction failure. */
 assert(HTTP_SettingsModelInit(NULL,save,NULL)>=0);
 uint16_t instance=0;const HTTP_SettingPage*page=HTTP_SettingsCatalogFind("/set",&instance);
 HTTP_SettingsBindingsCommitted(page,0);committed=0;
 uint8_t changed[33]={0},values[33]={0},expected[33]={0};
 changed[0]=changed[14]=1;values[0]=HTTP_MODE_PT1000;values[14]=HTTP_MODE_ADC;
 expected[0]=modes[0];expected[14]=modes[14];
 unsigned untouched=modes[1];
 uint32_t serial=HTTP_LCDPanelSaveModes(changed,values,expected);assert(serial);
 assert(!HTTP_LCDPanelSaveModes(changed,values,expected)&&HTTP_LCDPanelWanted());
 assert(modes[0]==HTTP_MODE_TEMP&&modes[14]==HTTP_MODE_DISABLED);
 HTTP_LCDPanelService(data,sizeof data,1600);HTTP_LCDPanelRead(&out);
 assert(modes[0]==HTTP_MODE_PT1000&&modes[14]==HTTP_MODE_ADC&&modes[1]==untouched&&committed==1);
 assert(out.mode_result==HTTP_SETTINGS_SAVED&&out.mode_serial==serial);
 expected[0]=modes[0];expected[14]=modes[14];values[0]=HTTP_MODE_INPUT;values[14]=HTTP_MODE_DISABLED;
 fail_save=1;assert(HTTP_LCDPanelSaveModes(changed,values,expected));
 HTTP_LCDPanelService(data,sizeof data,1700);HTTP_LCDPanelRead(&out);
 assert(modes[0]==HTTP_MODE_PT1000&&modes[14]==HTTP_MODE_ADC&&out.mode_result==HTTP_SETTINGS_STORAGE_ERROR&&committed==1);
 fail_save=0;expected[0]=HTTP_MODE_TEMP;assert(HTTP_LCDPanelSaveModes(changed,values,expected));
 HTTP_LCDPanelService(data,sizeof data,1800);HTTP_LCDPanelRead(&out);
 assert(modes[0]==HTTP_MODE_PT1000&&modes[14]==HTTP_MODE_ADC&&out.mode_result==HTTP_SETTINGS_INVALID);
 expected[0]=modes[0];values[14]=HTTP_MODE_RELAY;assert(HTTP_LCDPanelSaveModes(changed,values,expected));
 HTTP_LCDPanelService(data,sizeof data,1900);HTTP_LCDPanelRead(&out);
 assert(out.mode_result==HTTP_SETTINGS_INVALID&&committed==1&&modes[0]==HTTP_MODE_PT1000);
 values[14]=HTTP_MODE_DISABLED;assert(HTTP_LCDPanelSaveModes(changed,values,expected));
 HTTP_LCDPanelService(data,sizeof data,2000);assert(modes[0]==HTTP_MODE_INPUT&&modes[14]==HTTP_MODE_DISABLED&&committed==2);
 /* Later HTTP commit is published without re-entering the LCD panel. */
 ModeBatch web={0};web.changed[0]=1;web.modes[0]=HTTP_MODE_TEMP;web.expected[0]=HTTP_MODE_INPUT;
 HTTP_LCDPanelActive(1);assert(SaveModes(&web)==HTTP_SETTINGS_SAVED);
 HTTP_LCDPanelService(data,sizeof data,2500);HTTP_LCDPanelRead(&out);assert(out.ports[0].mode==HTTP_MODE_TEMP);
 assert(HTTP_SettingsModelInit(load,save,NULL)==1);
 assert(!strcmp(HTTP_SettingsGetByName(page,0,"n_GPIO_val_001"),"02"));
 assert(!strcmp(HTTP_SettingsGetByName(page,0,"n_s_Port_nazwa1_001"),"Nazwa portu nr 1"));
 memset(changed,0,sizeof changed);assert(!HTTP_LCDPanelSaveModes(changed,values,expected));
 assert(!HTTP_LCDPanelSaveModes(NULL,values,expected));
 assert(!critical);puts("PASS: shared settings/refresh values, bounded snapshots, latest PWM per channel, mode validation, relay toggle against HTTP feedback, no changes when inactive, commands survive EXIT.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for name in ['http_lcd_panels.c','http_lcd_panels.h','http_settings_model.c','http_settings_model.h','http_settings_catalog.c','http_settings_catalog.h','http_settings_bindings.h']:shutil.copy(ROOT/'Application/HTTP'/name,p/name)
 for name in ['relay_states.c','relay_states.h']:shutil.copy(ROOT/'Application'/name,p/name)
 (p/'wwwPages.h').write_text(mode+'\nunsigned HTTP_SettingsPortMode(unsigned);\nconst char*HTTP_SettingsPortName(unsigned,const char*);\n')
 (p/'http_sensor_lcd.h').write_text('void HTTP_SensorLCDWake(void);\n')
 (p/'FreeRTOS.h').write_text('void enter(void);void leave(void);\n#define taskENTER_CRITICAL() enter()\n#define taskEXIT_CRITICAL() leave()\n')
 (p/'task.h').write_text('')
 (p/'test.c').write_text(C)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+d,'-I'+str(ROOT/'Application/HTTP'),'-I'+str(ROOT/'Application'),str(p/'test.c'),str(p/'relay_states.c'),str(p/'http_settings_model.c'),str(p/'http_settings_catalog.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
