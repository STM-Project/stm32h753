#!/usr/bin/env python3
"""Real browser -> HTTP stream decoder -> production routing/model/templates.
Requires host GCC, Node/playwright and chromium (CHROMIUM_PATH may override).
The local bridge replaces only the ESP transport, not the request decoder,
page renderers, settings validation, or shared model.
"""
import ast
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import re
import subprocess
import sys
import threading
import test_http_sensor_routes as sensor

ROOT = sensor.ROOT
BRIDGE = r'''
static int bridge_save(const void *data,size_t length,void *context) {
    (void)data;(void)length;(void)context; return 1;
}
static void cache_checks(void) {
    static char before[250000],after[250000],snapshot[4096];
    request(0,HTTP_PANEL_SET_PATH);body(0,before,sizeof before);
    assert(strstr(httpPar.web[0][0],"max-age=31536000, immutable"));
    assert(RELAY_RequestPwm(21,88));assert(RELAY_RequestToggle(28));
    request(0,"/lpc.cgi/A117?h7_page=%2Fset&h7_form=0&n_s_Port_nazwa5_001=Changed&n_109_zapisz=");
    request(0,HTTP_PANEL_SET_PATH);body(0,after,sizeof after);
    assert(!strcmp(before,after)); /* no names/modes/PWM/relay state in cache */
    request(0,HTTP_PANEL_READ_PATH);body(0,before,sizeof before);
    assert(strstr(before,"h7-tile-11"));assert(!strstr(before,"Changed"));
    size_t length=HTTP_PanelConfigJson(before,sizeof before);
    assert(length && length==strlen(before));
    for(size_t capacity=0;capacity<=length+1;capacity++) {
        memset(after,0x55,length+3);
        size_t n=HTTP_PanelConfigJson(after+1,capacity);
        assert(after[0]==0x55 && after[capacity+1]==0x55);
        assert(capacity>length?n==length:n==0);
    }
    clear_queue();HTTP_HandleTarget(4,"/api/panel-config",17,NULL);
    body(0,snapshot,sizeof snapshot);
    assert(RELAY_RequestPwm(21,33));
    HTTP_HandleTarget(1,"/api/panel-config",17,NULL);
    body(0,before,sizeof before);assert(!strcmp(before,snapshot));
    body(1,after,sizeof after);assert(strcmp(after,snapshot));
    request(0,"/assets/h7-panels-old.js");
    assert(strstr(httpPar.web[0][0],"404 Not Found"));
    assert(strstr(httpPar.web[0][0],"Cache-Control: no-store"));
    HTTP_SettingsModelInit(NULL,bridge_save,NULL);RELAY_ResetStates();
}
int main(void) {
    (void)sensor_routes_suite;
    HTTP_SettingsModelInit(NULL,bridge_save,NULL);
    HTTP_SensorSettingsInit(NULL,bridge_save,NULL);
    cache_checks();
    static char line[200001],raw[100000],content[250000];
    while(fgets(line,sizeof line,stdin)) {
        size_t length=strcspn(line,"\r\n")/2;
        if(length>=sizeof raw)return 2;
        for(size_t i=0;i<length;i++) {unsigned n;assert(sscanf(line+2*i,"%2x",&n)==1);raw[i]=(char)n;}
        clear_queue();
        HTTP_RequestStream stream;HTTP_RequestStreamInit(&stream,HTTP_HandleTarget,NULL);
        for(size_t i=0;i<length;) {
            size_t n=length-i;if(n>1024)n=1024;
            char frame[40];int prefix=snprintf(frame,sizeof frame,"+IPD,0,%zu:",n);
            HTTP_RequestStreamFeed(&stream,(const uint8_t*)frame,(size_t)prefix);
            HTTP_RequestStreamFeed(&stream,(const uint8_t*)raw+i,n);i+=n;
        }
        int slot=slot_for(0);assert(slot>=0);
        size_t n=body((unsigned)slot,content,sizeof content);
        const char *header=httpPar.web[slot][0];
        size_t h=strlen(header);
        assert(h==httpPar.siz[slot][0]);
        printf("%zu %zu\n",h,n);fwrite(header,1,h,stdout);fwrite(content,1,n,stdout);fflush(stdout);
    }
    return 0;
}
'''

def browser(executable):
    process = subprocess.Popen([str(executable)], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    lock = threading.Lock()
    stats = []
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self): self.serve()
        def do_POST(self): self.serve()
        def log_message(self, *args): pass
        def serve(self):
            body = self.rfile.read(int(self.headers.get('Content-Length', '0')))
            request = (self.command + ' ' + self.path + ' HTTP/1.1\r\n' + str(self.headers).replace('\n', '\r\n').rstrip('\r\n') + '\r\n\r\n').encode() + body
            with lock:
                process.stdin.write(request.hex().encode() + b'\n'); process.stdin.flush()
                line = process.stdout.readline()
                if not line: raise RuntimeError('C bridge stopped')
                h, n = map(int, line.split())
                header, content = process.stdout.read(h), process.stdout.read(n)
                stats.append((self.path, n, header.decode()))
            self.wfile.write(header + content)
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True);thread.start()
    try:
        subprocess.run(['node', str(ROOT / 'tests/fixtures/http_panel_cache_browser.js'), str(server.server_port)], check=True)
        assets = [(path, n) for path,n,h in stats if path.startswith('/assets/')]
        assert len(assets) == 3, assets  # one script, one read layout, one set layout across revisits
        for path,n,h in stats:
            if path.startswith('/assets/'):
                assert 'max-age=31536000, immutable' in h
                assert 'Cache-Control: no-store' not in h
            else:
                assert 'Cache-Control: no-store' in h
            if path in ('/main','/set','/'):
                assert n < 1000,(path,n)
        print('Network payloads:', assets)
        print('PASS: cached static layouts, fresh shared JSON, settings/PWM/relay actions, remote changes and dirty draft protection')
    finally:
        server.shutdown(); server.server_close()
        process.stdin.close(); process.wait(timeout=10)
        assert process.returncode == 0

def main():
    subprocess.run([sys.executable, str(ROOT / "tools/generate_http_panel_assets.py"), "--check"], check=True)
    source = sensor.harness_source(sensor.TESTS.replace('int main(void)', 'int sensor_routes_suite(void)') + BRIDGE)
    # Feed the production TME data to the retained polling scripts.
    pages = (ROOT / 'Application/wwwPages.c').read_text()
    literal = re.search(r'char HttpRefr\[\]="(.*?)";', pages, re.S)[1]
    refresh = ast.literal_eval('"' + literal + '"')
    source = source.replace('static char HttpRefr_sdram[2048]="refresh";', 'static char HttpRefr_sdram[2048]=' + json.dumps(refresh) + ';')
    sensor.compile_and_run(source, browser)

if __name__ == '__main__': main()
