#include "lpc_pages.h"
#include "http_lpc_panel.h"
#include "../HTTP/http_sensor_settings.h"
#include "../HTTP/http_settings_model.h"
#include "../relay_states.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static char value[257];
static const char *port_name(unsigned port){uint16_t instance=0;const HTTP_SettingPage *page=HTTP_SettingsCatalogFind("/set",&instance);char name[40];snprintf(name,sizeof name,"n_s_Port_nazwa%u_001",port);return HTTP_SettingsGetByName(page,instance,name);}
const char *LPC_DefaultFromH7(const char *name)
{
    unsigned port=0;char suffix[16]={0};
    if(sscanf(name,"n_txtval%u%15s",&port,suffix)==1 && port && port<=20)return HTTP_SensorSettingsGet(port,0)->name;
    if(sscanf(name,"n_txtwe%u%15s",&port,suffix)==1 && port && port<=HTTP_LPC_INPUT_COUNT)return port_name(HTTP_LpcInputPorts[port-1]);
    if(sscanf(name,"n_pktxt%u%15s",&port,suffix)==1 && port && port<=RELAY_OUTPUT_COUNT)return port_name(RELAY_GetStates()->outputs[port-1].port);
    const char *prefix[]={"n_prgmaxval","n_prgminval","n_histval","n_korval"};
    for(unsigned i=0;i<4;i++)if(!strncmp(name,prefix[i],strlen(prefix[i]))){char *end;unsigned p=(unsigned)strtoul(name+strlen(prefix[i]),&end,10);if(*end || !p || p>20)return NULL;const HttpSensorSettings *s=HTTP_SensorSettingsGet(p,0);int n=i==0?s->max1:i==1?s->min1:i==2?s->hysteresis:s->correction;unsigned magnitude=(unsigned)(n<0?-n:n);snprintf(value,sizeof value,"%s%u.%u",n<0?"-":"",magnitude/10,magnitude%10);return value;}
    const char *lpc[]={"n_lan_01_mac","n_lan_01_ip","n_lan_01_mask","n_lan_01_br","n_lan_01_port"};
    const char *h7[]={"n_s_Lan_mac_001","n_s_Lan_ip_001","n_s_Lan_mask_001","n_s_Lan_br_001","n_s_Lan_port_001"};
    for(unsigned i=0;i<5;i++)if(!strcmp(name,lpc[i])){uint16_t instance=0;const HTTP_SettingPage *page=HTTP_SettingsCatalogFind("/lan",&instance);return HTTP_SettingsGetByName(page,instance,h7[i]);}
    return NULL;
}
