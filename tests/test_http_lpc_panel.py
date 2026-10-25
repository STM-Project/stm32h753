#!/usr/bin/env python3
"""Exercise actual LPC assets, sensor publications, JSON and queued HTTP routes."""
import test_http_sensor_routes as sensor
import test_http_settings_routes as settings
TESTS=r'''
static int discard_save(const void *bytes,size_t n,void *ctx) {(void)bytes;(void)n;(void)ctx;return 1;}
int main(void) {
    static char data[20000], snapshot[20000];
    HTTP_SettingsModelInit(NULL,discard_save,NULL);
    HTTP_SensorSettingsInit(NULL,NULL,NULL);
    RELAY_ResetStates();HTTP_LpcResetMeasurements();
    request(0,"/nowa");assert(strstr(httpPar.web[0][0],"HTTP/1.1 200 OK"));
    const HTTP_LpcAsset *page=HTTP_LpcFindAsset("/nowa",5);
    assert(page && strstr(page->data,"id=\"fixture3\"") && !strstr(page->data,"<%"));
    assert(!HTTP_LpcFindAsset("/lpc/../index.html",18));
    assert(page->length>400000u && httpPar.siz[0][1]==page->length);
    assert(!strstr(page->data,"<script src="));
    assert(!strstr(page->data,"<link rel=\"stylesheet\""));
    assert(strstr(page->data,"window.HTTP_LPC_SVG="));
    assert(!HTTP_LpcFindAsset("/lpc/panel.js",13));
    assert(httpPar.web[0][1]==page->data);
    request(0,"/api/nowa");check_response(0,200,"application/json; charset=UTF-8");
    body(0,data,sizeof data);assert(strstr(data,"\"tenths\":null"));assert(strstr(data,"\"state\":null"));
    for(unsigned i=0;i<HTTP_LPC_TEMPERATURE_COUNT;i++)assert(HTTP_LpcSetTemperature(i+1,251+(int)i,1));
    assert(HTTP_LpcSetInput(4,1,1));assert(HTTP_LpcSetInput(10,0,1));
    assert(!HTTP_LpcSetTemperature(0,200,1));assert(!HTTP_LpcSetTemperature(1,1501,1));assert(!HTTP_LpcSetInput(4,2,1));
    assert(RELAY_UpdateState(28,1));
    assert(HTTP_LpcPanelJson(data,sizeof data));assert(strstr(data,"\"tenths\":251"));
    assert(strstr(data,"\"state\":true,\"requested\":null"));
    for(size_t n=1;n<strlen(data)+1;n++){static char small[20000];assert(!HTTP_LpcPanelJson(small,n) && !small[0]);}
    assert(!HTTP_LpcPanelJson(NULL,10));
    /* Save an escaped name and a relay mode through the same settings decoder. */
    const HTTP_SettingPage *settings=NULL;uint16_t instance=0;
    generic_form("/set",0,0,0);
    const char *target=generic_target;
    assert(HTTP_SettingsApplyTarget(target,strlen(target),&settings,&instance)==HTTP_SETTINGS_SAVED);
    main_mode_override[28]="00";generic_form("/set",1,0,0);target=generic_target;
    assert(HTTP_SettingsApplyTarget(target,strlen(target),&settings,&instance)==HTTP_SETTINGS_SAVED);
    HTTP_LpcPanelJson(data,sizeof data);assert(strstr(data,"Kuchnia & <ok> Łódź"));assert(strstr(data,"\"enabled\":true"));
    /* Use the actual legacy SDRAM layout, not the separate publication API. */
    memset(HttpRefr_sdram,' ',120);HttpRefr_sdram[120]=0;
    memcpy(HttpRefr_sdram+25,"0123",4);
    memcpy(HttpRefr_sdram+39,"111023101111111111",18);
    memcpy(HttpRefr_sdram+73," 25.1",5);memcpy(HttpRefr_sdram+85,"-12.3",5);memcpy(HttpRefr_sdram+97," 40.5",5);
    request(0,"/api/nowa");body(0,data,sizeof data);
    assert(strstr(data,"\"tenths\":251") && strstr(data,"\"tenths\":-123") && strstr(data,"\"tenths\":405"));
    /* Active-low: input port 4 has code 0 (ON), port 5 code 2 (ON). */
    const char *input=strstr(data,"\"port\":4,");const char *state=strstr(input,"\"state\":true");
    assert(state && state<strchr(input,'}'));
    input=strstr(data,"\"port\":6,");state=strstr(input,"\"state\":false");assert(state && state<strchr(input,'}'));
    for(size_t length=0;length<121;length++)assert(HTTP_LpcPanelJsonFromRefresh(data,sizeof data,HttpRefr_sdram,length));
    memcpy(HttpRefr_sdram+85,"-78.3",5);memcpy(HttpRefr_sdram+97,"-56.0",5);
    request(0,"/api/nowa");body(0,data,sizeof data);
    assert(strstr(data,"\"tenths\":-783") && strstr(data,"\"tenths\":-560"));
    memcpy(HttpRefr_sdram+73," --.-",5);request(0,"/api/nowa");body(0,data,sizeof data);assert(strstr(data,"\"tenths\":null"));
    memcpy(HttpRefr_sdram+73," 25.1",5);
    clear_queue();HTTP_HandleTarget(2,"/api/nowa",9,NULL);body(0,snapshot,sizeof snapshot);
    memcpy(HttpRefr_sdram+73,"-05.5",5);HTTP_HandleTarget(1,"/api/nowa",9,NULL);
    body(0,data,sizeof data);assert(strcmp(data,snapshot));assert(strstr(data,"\"tenths\":-55"));
    strcpy(snapshot,data);body(1,data,sizeof data);assert(!strcmp(data,snapshot));
    request(0,"/mobile/pk001");check_response(0,200,"text/html; charset=UTF-8");
    HTTP_LpcPanelJson(data,sizeof data);assert(strstr(data,"\"state\":false,\"requested\":true"));
    assert(HTTP_LpcSetTemperature(1,0,0));assert(HTTP_LpcSetInput(4,0,0));
    HTTP_LpcPanelJson(data,sizeof data);assert(strstr(data,"\"tenths\":null"));
    puts("PASS: offline LPC resources, /nowa, JSON publication, unknown/actual/requested states, mode guards, escaped settings names, bounds and queued snapshot isolation.");
    return 0;
}
'''
def main():
    suite=sensor.TESTS.replace('int main(void)','static int __attribute__((unused)) sensor_routes_suite(void)')
    suite+=settings.TESTS.replace('int main(void)','static int __attribute__((unused)) settings_suite(void)')
    sensor.compile_and_run(sensor.harness_source(suite+TESTS))
if __name__=='__main__':main()
