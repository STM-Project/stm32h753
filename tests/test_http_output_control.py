#!/usr/bin/env python3
"""Real GET routes, mode validation, queued replies and relay/PWM variables."""
import test_http_sensor_routes as sensor
import test_http_settings_routes as settings
from test_settings_templates import literal
import json
import re
import subprocess

TESTS = r'''
int main(void) {
    static char html[250000], first[250000];
    assert(settings_suite()==0);
    RELAY_ResetStates();
    assert(HTTP_SettingsModelInit(NULL,generic_save,generic_persisted)==0);
    memset(main_mode_override,0,sizeof main_mode_override);
    main_mode_override[21]=main_mode_override[22]=main_mode_override[23]="01";
    generic_form("/set",1,0,0);request(0,generic_target);
    unsigned saves_before=generic_saves;
    const RelayState *pwm=RELAY_GetState(21), *pk=RELAY_GetState(28);
    const char raw[]="/lpc.cgi/n_pwm1=37&n_115_zapisz=";
    HTTP_OutputCommand parsed;
    for(size_t length=0;length<sizeof raw-1u;length++)
        assert(HTTP_OutputControlParse(raw,length,&parsed)!=1);
    assert(HTTP_OutputControlParse(raw,sizeof raw-1u,&parsed)==1);
    assert(parsed.kind==HTTP_OUTPUT_PWM && parsed.port==21 && parsed.value==37);
    request(0,"/lpc.cgi/n_pwm1=37&n_115_zapisz=");
    check_response(0,200,"text/plain; charset=UTF-8");body(0,html,sizeof html);assert(!strcmp(html,"37"));
    assert(pwm->pwm_valid && pwm->pwm_percent==37 && pwm->requested_on && pwm->request_valid);
    assert(!pwm->valid && !pwm->on); /* A request is not physical GPIO feedback. */
    request(0,"/main");body(0,html,sizeof html);
    assert(strstr(html,"value=\"37\""));
    request(0,"/set");body(0,html,sizeof html);assert(strstr(html,"value=\"37\""));
    request(0,"/mobile/pk001");check_response(0,200,"text/html; charset=UTF-8");
    body(0,html,sizeof html);assert(strstr(html,"Stan ON"));assert(pk->request_valid && pk->requested_on && !pk->valid);
    unsigned revision=RELAY_GetStates()->revision;
    HTTP_HandleTarget(0,"/mobile/pk001",13,NULL);assert(RELAY_GetStates()->revision==revision);
    request(0,"/mobile/pk001");body(0,html,sizeof html);assert(strstr(html,"Stan OFF") && !pk->requested_on);
    assert(generic_saves==saves_before); /* Commands never write settings/SD. */

    RelayStates snapshot=*RELAY_GetStates();
    const char *invalid[]={"/lpc.cgi/n_pwm0=1&n_115_zapisz=","/lpc.cgi/n_pwm4=1&n_115_zapisz=",
        "/lpc.cgi/n_pwm1=-1&n_115_zapisz=","/lpc.cgi/n_pwm1=101&n_115_zapisz=",
        "/lpc.cgi/n_pwm1=1.5&n_115_zapisz=","/lpc.cgi/n_pwm1=1000&n_115_zapisz=",
        "/lpc.cgi/n_pwm1=&n_115_zapisz=","/lpc.cgi/n_pwm1=1&n_115_zapisz=x",
        "/lpc.cgi/n_pwm1=1&n_115_zapisz=&extra=1","/mobile/pk000","/mobile/pk014","/mobile/pk001x"};
    for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++) {
        request(0,invalid[i]);check_response(0,400,"text/html; charset=UTF-8");
        assert(!memcmp(&snapshot,RELAY_GetStates(),sizeof snapshot));
    }
    for(unsigned channel=1;channel<=3;channel++) {
        char path[80];snprintf(path,sizeof path,"/lpc.cgi/n_pwm%u=100&n_115_zapisz=",channel);
        request(0,path);assert(RELAY_GetState(channel+20)->pwm_percent==100);
        snprintf(path,sizeof path,"/lpc.cgi/n_pwm%u=0&n_115_zapisz=",channel);
        request(0,path);assert(RELAY_GetState(channel+20)->pwm_percent==0);
        assert(!RELAY_GetState(channel+20)->requested_on);
    }
    /* Exercise the original button mapping for every one of the 13 outputs. */
    for(unsigned port=21;port<=33;port++)main_mode_override[port]="00";
    generic_form("/set",1,0,0);request(0,generic_target);RELAY_ResetStates();
    for(unsigned instance=1;instance<=13;instance++) {
        char path[32];snprintf(path,sizeof path,"/mobile/pk%03u",instance);
        request(0,path);check_response(0,200,"text/html; charset=UTF-8");
        assert(RELAY_GetStates()->outputs[instance-1].requested_on);
        request(0,path);assert(!RELAY_GetStates()->outputs[instance-1].requested_on);
    }
    /* Reject a slider in relay mode and a manual toggle in timer mode. */
    snapshot=*RELAY_GetStates();request(0,"/lpc.cgi/n_pwm1=50&n_115_zapisz=");
    check_response(0,400,"text/html; charset=UTF-8");assert(!memcmp(&snapshot,RELAY_GetStates(),sizeof snapshot));
    main_mode_override[28]="10";generic_form("/set",1,0,0);request(0,generic_target);
    snapshot=*RELAY_GetStates();request(0,"/mobile/pk001");check_response(0,400,"text/html; charset=UTF-8");
    assert(!memcmp(&snapshot,RELAY_GetStates(),sizeof snapshot));
    main_mode_override[28]="00";main_mode_override[21]="01";
    generic_form("/set",1,0,0);request(0,generic_target);

    /* A fragmented, bounded ESP GET reaches the same state API exactly once. */
    HTTP_RequestStream stream;clear_queue();HTTP_RequestStreamInit(&stream,HTTP_HandleTarget,NULL);
    feed_request(&stream,2,"/lpc.cgi/n_pwm1=64&n_115_zapisz=",1);
    check_response(2,200,"text/plain; charset=UTF-8");assert(pwm->pwm_percent==64);
    /* Each queued acknowledgement retains its own state snapshot. */
    clear_queue();HTTP_HandleTarget(4,"/mobile/pk001",13,NULL);body(0,first,sizeof first);
    HTTP_HandleTarget(3,"/mobile/pk001",13,NULL);body(0,html,sizeof html);assert(!strcmp(first,html));
    body(1,html,sizeof html);assert(strcmp(first,html));
    while(slot_for(0)<0)HTTP_HandleTarget(0,"/favicon.ico",12,NULL);
    HTTP_HandleTarget(1,"/favicon.ico",12,NULL);HTTP_HandleTarget(2,"/favicon.ico",12,NULL);
    snapshot=*RELAY_GetStates();HTTP_HandleTarget(5,"/mobile/pk001",13,NULL);
    HTTP_HandleTarget(0,"/mobile/pk001",13,NULL);assert(!memcmp(&snapshot,RELAY_GetStates(),sizeof snapshot));
    /* Polling uses the same requested states, and never aliases its mutable source. */
    memset(HttpRefr_sdram,'0',200);HttpRefr_sdram[200]=0;
    request(0,"/mobile/pk001");assert(pk->requested_on);
    request(0,"/TME.txt");body(0,first,sizeof first);
    assert(first[25]=='1' && first[35]=='1');
    HttpRefr_sdram[0]='x';body(0,html,sizeof html);assert(!strcmp(first,html));
    puts("PASS: real PWM/button GET updates, 13-output mapping, 0..100 limits, mode guards, queued/fragmented requests, retained slider values and polling snapshots.");
    return 0;
}
'''


def main():
    suites = (sensor.TESTS.replace("int main(void)", "static int sensor_routes_suite(void)") +
              settings.TESTS.replace("int main(void)", "static int settings_suite(void)"))
    sensor.compile_and_run(sensor.harness_source(suites + TESTS))
    source = (sensor.ROOT / "Application/wwwPages.c").read_text()
    for name in ("HttpStructMainReadPanel", "HttpStructMainReadPanel2"):
        scripts = re.findall(r"<script[^>]*>(.*?)</script>", literal(source, name), re.S)
        javascript = r'''
const vm=require('node:vm'),assert=require('node:assert/strict');
const requests=[],nodes={},forms={};
for(let i=1;i<=33;i++) {const id=String(i).padStart(3,'0');forms['wybor'+id]={['n_GPIO_val_'+id]:{value:'06p'+id}};}
function XHR(){requests.push(this);this.open=(method,url)=>{this.url=url;};this.send=()=>{};}
const context={XMLHttpRequest:XHR,setInterval:()=>0,
 document:{forms,getElementById:id=>nodes[id]??(nodes[id]={innerHTML:'',style:{}}),querySelector:()=>({value:0})}};
context.window=context;vm.createContext(context);
for(const script of SCRIPTS)vm.runInContext(script,context);
context.wst=1;context.x021('37');
assert.equal(requests.at(-1).url,'/lpc.cgi/n_pwm1=37&n_115_zapisz=');
const pwm=requests.at(-1),count=requests.length;
context.x021('48');context.x021('55');assert.equal(requests.length,count);
pwm.readyState=4;pwm.status=200;pwm.onreadystatechange();
assert.equal(requests.length,count+1);
assert.equal(requests.at(-1).url,'/lpc.cgi/n_pwm1=55&n_115_zapisz=');
context.xpk028();const first=requests.at(-1);assert.equal(first.url,'/mobile/pk001');
context.xpk029();const second=requests.at(-1);assert.equal(second.url,'/mobile/pk002');
first.readyState=4;first.status=200;first.responseText='<font>Stan ON</font>';first.onreadystatechange();
assert.equal(nodes.tu_pk028.innerHTML,first.responseText);assert.notEqual(nodes.tu_pk029?.innerHTML,first.responseText);
second.readyState=4;second.status=200;second.responseText='<font>Stan OFF</font>';second.onreadystatechange();
assert.equal(nodes.tu_pk029.innerHTML,second.responseText);
'''.replace("SCRIPTS", json.dumps(scripts))
        subprocess.run(["node", "-e", javascript], check=True)
    print("PASS: both pages send during polling, serialize the latest PWM value and update the correct overlapping button response.")


if __name__ == "__main__":
    main()
