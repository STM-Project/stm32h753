#include "http_lpc_panel.h"
#include "http_sensor_settings.h"
#include "http_settings_model.h"
#include "../relay_states.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

const uint8_t HTTP_LpcTemperaturePorts[3] = {1,2,3};
const uint8_t HTTP_LpcInputPorts[7] = {4,5,6,7,8,9,10};
const uint8_t HTTP_LpcOutputPorts[4] = {28,29,30,31};
HTTP_LpcMeasurements HTTP_LpcMeasurementsData;

void HTTP_LpcResetMeasurements(void) { memset(&HTTP_LpcMeasurementsData,0,sizeof HTTP_LpcMeasurementsData); }
int HTTP_LpcSetTemperature(unsigned port, int tenths, int valid)
{
    if (valid && (tenths < -550 || tenths > 1500)) return 0;
    for (unsigned i=0;i<HTTP_LPC_TEMPERATURE_COUNT;i++) if (HTTP_LpcTemperaturePorts[i]==port) {
        HTTP_LpcMeasurementsData.temperature_tenths[i]=(int16_t)(valid?tenths:0);
        HTTP_LpcMeasurementsData.temperature_valid[i]=(uint8_t)!!valid;
        return 1;
    }
    return 0;
}
int HTTP_LpcSetInput(unsigned port, int on, int valid)
{
    if (valid && on!=0 && on!=1) return 0;
    for (unsigned i=0;i<HTTP_LPC_INPUT_COUNT;i++) if (HTTP_LpcInputPorts[i]==port) {
        HTTP_LpcMeasurementsData.input_on[i]=(uint8_t)(valid?on:0);
        HTTP_LpcMeasurementsData.input_valid[i]=(uint8_t)!!valid;
        return 1;
    }
    return 0;
}
typedef struct {char *out;size_t size,used;int failed;} Writer;
static void append(Writer *w,const char *format,...)
{
    if(w->failed)return;
    va_list args;va_start(args,format);
    int n=vsnprintf(w->out+w->used,w->size-w->used,format,args);va_end(args);
    if(n<0 || (size_t)n>=w->size-w->used)w->failed=1;
    else w->used+=(size_t)n;
}
static void string(Writer *w,const char *s)
{
    append(w,"\"");
    for(;s && *s;s++){
        unsigned char c=(unsigned char)*s;
        if(c=='"' || c=='\\')append(w,"\\%c",c);
        else if(c<32)append(w,"\\u%04x",c);
        else append(w,"%c",c);
    }
    append(w,"\"");
}
static const char *port_field(unsigned port,int mode)
{
    uint16_t instance=0;char name[40];
    const HTTP_SettingPage *page=HTTP_SettingsCatalogFind("/set",&instance);
    snprintf(name,sizeof name,mode?"n_GPIO_val_%03u":"n_s_Port_nazwa%u_001",port);
    return HTTP_SettingsGetByName(page,instance,name);
}
size_t HTTP_LpcPanelJson(char *out,size_t capacity)
{
    if(!out || !capacity)return 0;
    Writer w={out,capacity,0,0};out[0]=0;
    append(&w,"{\"temperatures\":[");
    for(unsigned i=0;i<HTTP_LPC_TEMPERATURE_COUNT;i++){
        unsigned port=HTTP_LpcTemperaturePorts[i];
        const HttpSensorSettings *s=HTTP_SensorSettingsGet(port,0);
        append(&w,"%s{\"port\":%u,\"name\":",i?",":"",port);string(&w,s?s->name:"Temperatura");
        append(&w,",\"tenths\":");
        if(HTTP_LpcMeasurementsData.temperature_valid[i])append(&w,"%d",HTTP_LpcMeasurementsData.temperature_tenths[i]+(s?s->correction:0));
        else append(&w,"null");
        append(&w,",\"min\":%d,\"max\":%d}",s?s->min1:-100,s?s->max1:500);
    }
    append(&w,"],\"inputs\":[");
    for(unsigned i=0;i<HTTP_LPC_INPUT_COUNT;i++){
        unsigned port=HTTP_LpcInputPorts[i];
        append(&w,"%s{\"port\":%u,\"name\":",i?",":"",port);string(&w,port_field(port,0));
        append(&w,",\"state\":%s}",!HTTP_LpcMeasurementsData.input_valid[i]?"null":HTTP_LpcMeasurementsData.input_on[i]?"true":"false");
    }
    append(&w,"],\"outputs\":[");
    for(unsigned i=0;i<HTTP_LPC_OUTPUT_COUNT;i++){
        unsigned port=HTTP_LpcOutputPorts[i];const RelayState *s=RELAY_GetState(port);
        const char *mode=port_field(port,1);
        int enabled=mode && (strtoul(mode,NULL,10)==0 || strtoul(mode,NULL,10)==8);
        append(&w,"%s{\"port\":%u,\"command\":%u,\"name\":",i?",":"",port,i+1);string(&w,port_field(port,0));
        append(&w,",\"state\":%s,\"requested\":%s,\"enabled\":%s}",
            !s || !s->valid?"null":s->on?"true":"false",
            !s || !s->request_valid?"null":s->requested_on?"true":"false",
            enabled?"true":"false");
    }
    append(&w,"]}");
    if(w.failed){out[0]=0;return 0;}return w.used;
}
