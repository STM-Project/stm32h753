#!/usr/bin/env python3
"""Exercise the real ESP HTTP routing, queued pages and sensor form round trip.

The hardware-free harness extracts the production HTTP response functions and
settings templates. The read-panel renderer and its templates are real; only
number formatting, hardware bindings and the reload timer are stubbed.
Run: python3 tests/test_http_sensor_routes.py
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def constant(source, name):
    pattern = r'const char\s+' + name + r'\[\]\s*=\s*(?:"(?:\\.|[^"\\])*"\s*)+;'
    match = re.search(pattern, source, re.S)
    assert match, f"Missing HTML fragment: {name}"
    return match.group()


PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mini_printf.h"
#include "string_oper.h"
#include "http_request_stream.h"
#include "http_sensor_page.h"
#include "http_sensor_settings.h"
#include "http_settings_model.h"
#include "http_settings_page.h"
#include "http_output_control.h"
#include "http_lpc_panel.h"
#include "lpc_pages.h"
#include "relay_states.h"
typedef uint8_t u8;
typedef uint32_t u32;
#define SDRAM
static char last_debug[256];
static void DbgVarDma(int on,unsigned size,const char *format,...) {
    (void)size;if(!on)return;
    va_list args;va_start(args,format);mini_vsnprintf(last_debug,sizeof last_debug,format,args);va_end(args);
}
#define LOOP_FOR(i,n) for (int i=0;i<(n);++i)
static unsigned reload_count, bindings_count;
static void HTTP_SettingsBindingsCommitted(const HTTP_SettingPage *page, uint16_t instance) {
    assert(page); (void)instance; ++bindings_count;
}
static void InitReloadHttpTime(void) { ++reload_count; }
static int HTTP_SettingsStorageRetryLoad(void) { return 0; }
static const char *HTTP_SettingsStorageGetErrorMessage(void) {
    return "Nie zapisano ustawien. Sprawdz karte SD.";
}
/* Formatting normally uses a mutex-protected STM32 scratch buffer. Keep only
 * that platform dependency out of this harness, not the layout or mode logic. */
char *Float2Str(float value, char fill, int digits, int sign, int decimals) {
    static char values[8][64];
    static unsigned next;
    char *out = values[next++ % 8u];
    (void)fill; (void)digits; (void)sign;
    snprintf(out, sizeof values[0], "%.*f", decimals, (double)value);
    return out;
}
unsigned HTTP_SettingsPortMode(unsigned port);
const char *HTTP_SettingsPortName(unsigned port, const char *fallback);
char *HTTP_TEMPLATE_Hidden2(char *out, u8 type, u8 port);
static char HttpRefr_sdram[2048]="refresh";
static int lpc_force_json_stream;
static size_t test_lpc_json(unsigned page,char *out,size_t capacity){
    return lpc_force_json_stream?0:LPC_SettingsJson(page,out,capacity);
}
#define LPC_SettingsJson(...) test_lpc_json(__VA_ARGS__)
static const char *lpc_test_page,*lpc_test_page2;
static int lpc_chunk_fail;
static const char *test_sd_page(unsigned page){return page==2&&lpc_test_page2?lpc_test_page2:lpc_test_page;}
const char *LPC_StoragePageError(void){return "brak pliku testowego";}
size_t LPC_StoragePageSize(unsigned page){const char *p=test_sd_page(page);return p?strlen(p):0;}
size_t LPC_StorageReadPageChunk(unsigned page,size_t expected,size_t offset,char *out,size_t length){
    const char *p=test_sd_page(page);if(lpc_chunk_fail||!p||strlen(p)!=expected||offset>expected||length>expected-offset)return 0;
    memcpy(out,p+offset,length);return length;
}

'''

TESTS = r'''
static unsigned char persisted[HTTP_SENSOR_SETTINGS_BLOB_SIZE];
static unsigned saves;
static int saved, storage_failure;

static int load(void *out, size_t size, void *context) {
    assert(context == persisted); assert(size == sizeof persisted);
    if (!saved) return 0;
    memcpy(out, persisted, size); return 1;
}
static int save(const void *data, size_t size, void *context) {
    assert(context == persisted); assert(size == sizeof persisted);
    ++saves;
    if (storage_failure) return -1;
    assert(HTTP_SensorSettingsValidateBlob(data, size));
    memcpy(persisted, data, size); saved = 1; return 1;
}
static void clear_queue(void) {
    memset(&httpPar, 0, sizeof httpPar);
    memset(httpLpcPage,0,sizeof httpLpcPage);
    memset(httpLpcChunkOffset,0,sizeof httpLpcChunkOffset);
    memset(httpRefreshPage,0,sizeof httpRefreshPage);
    memset(httpRefreshPrepared,0,sizeof httpRefreshPrepared);
    memset(httpResponseStarted,0,sizeof httpResponseStarted);
    memset(httpPar.que, 0xff, sizeof httpPar.que);
}
static int slot_for(unsigned channel) {
    for (unsigned i = 0; i < ESP_MAX_HTTP_CONN; ++i)
        if (httpPar.que[i] == channel) return (int)i;
    return -1;
}
static size_t body(unsigned slot, char *out, size_t capacity) {
    HTTP_PrepareRefresh(slot);
    size_t length = 0;
    for (unsigned i = 1; i < MAX_HTML_WEBs && httpPar.web[slot][i]; ++i) {
        size_t size = httpPar.siz[slot][i];
        if (length + size >= capacity)
            fprintf(stderr, "Response exceeds test buffer: slot=%u part=%u total=%zu part=%zu capacity=%zu\n", slot, i, length, size, capacity);
        assert(length + size < capacity);
        if(httpLpcPage[slot]&&i==1)assert(LPC_StorageReadPageChunk(httpLpcPage[slot],size,0,out+length,size)==size);
        else memcpy(out + length, httpPar.web[slot][i], size);
        length += size;
    }
    out[length] = 0; return length;
}
static void check_response(unsigned channel, unsigned status, const char *type) {
    char expected[100], content[250000];
    int slot = slot_for(channel);
    assert(slot >= 0);
    HTTP_PrepareRefresh((unsigned)slot);
    const char *header = httpPar.web[slot][0];
    size_t length = body((unsigned)slot, content, sizeof content);
    snprintf(expected, sizeof expected, "HTTP/1.1 %u ", status);
    assert(strncmp(header, expected, strlen(expected)) == 0);
    snprintf(expected, sizeof expected, "Content-Type: %s\r\n", type);
    assert(strstr(header, expected));
    snprintf(expected, sizeof expected, "Content-Length: %zu\r\n", length);
    assert(strstr(header, expected));
    assert(strstr(header, "Connection: close\r\n"));
    assert(strstr(header, "Cache-Control: no-store\r\n"));
    assert(strlen(header) == httpPar.siz[slot][0]);
    assert(strlen(content) == length);
    assert(strstr(header, "\r\n\r\n"));
}
static void request(unsigned channel, const char *target) {
    clear_queue(); HTTP_HandleTarget(channel, target, strlen(target), NULL);
}
static void sensor(char *target, unsigned id, int humidity) {
    size_t used = strlen(target);
    int size = snprintf(target + used, HTTP_SENSOR_TARGET_MAX - used,
        "n_s_Czujki_nazwa_%03u=%s&"
        "n_s_Czujki_prog_max1_%03u=%s&n_s_Czujki_prog_max2_%03u=%s&"
        "n_s_Czujki_prog_min1_%03u=%s&n_s_Czujki_prog_min2_%03u=%s&"
        "n_s_Czujki_hist_%03u=0.5&n_s_Czujki_korekcja_%03u=-1.2&"
        "n_s_Czujki_czas_reak_%03u=01%%3A30&",
        id, humidity ? "Wilgotnosc" : "Salon+%26+%22kuchnia%22", id,
        humidity ? "85.0" : "%2B26.1", id, humidity ? "90.0" : "31.0",
        id, humidity ? "10.0" : "-15.2", id, humidity ? "5.0" : "-20.1",
        id, id, id);
    assert(size > 0 && (size_t)size < HTTP_SENSOR_TARGET_MAX - used);
}
static void form(char *target, unsigned port, int humidity) {
    strcpy(target, "/lpc.cgi/A117?"); sensor(target, 2 * port - 1, 0);
    if (humidity) sensor(target, 2 * port, 1);
    strcat(target, "n_109_zapisz=");
}
static void feed_request(HTTP_RequestStream *stream, unsigned channel,
                         const char *target, unsigned chunk) {
    char request[4096], frame[4200];
    int size;
    if (strncmp(target,"/lpc.cgi/A117?",14)==0)
        size=snprintf(request,sizeof request,"POST /lpc.cgi/A117 HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %zu\r\n\r\n%s",strlen(target+14),target+14);
    else size=snprintf(request,sizeof request,"GET %s HTTP/1.1\r\nHost: stm32\r\n\r\n",target);
    assert(size > 0 && (size_t)size < sizeof request);
    size = snprintf(frame, sizeof frame, "+IPD,%u,%d:%s", channel, size, request);
    assert(size > 0 && (size_t)size < sizeof frame);
    for (size_t i = 0; i < (size_t)size; i += chunk) {
        size_t count = (size_t)size - i;
        if (count > chunk) count = chunk;
        HTTP_RequestStreamFeed(stream, (const uint8_t *)frame + i, count);
    }
}

/* Explicit RHU test fixture: production defaults now match /nowa. */
static const char *sensor_test_mode(const HTTP_SettingPage *page,uint16_t instance,
                                   const HTTP_SettingField *field,void *context) {
    (void)page;(void)instance;(void)context;
    return !strcmp(field->name,"n_GPIO_val_005")?"09":NULL;
}
int main(void) {
    HTTP_SettingsSetDefaultResolver(sensor_test_mode,NULL);
    static char html[250000], first[250000], target[HTTP_SENSOR_TARGET_MAX];
    HTTP_RequestStream stream;
    assert(HTTP_REQUEST_STREAM_CHANNELS >= ESP_MAX_HTTP_CONN);
    assert(HTTP_SensorSettingsInit(load, save, persisted) == 0);
    assert(HTTP_SettingsModelInit(NULL, NULL, NULL) == 0);
    lpc_test_page="<!doctype html><p>SD LPC OK</p>";
    for(unsigned page=1;page<=LPC_PAGE_COUNT;page++){
        char path[32];snprintf(path,sizeof path,"/nowa%u",page);
        request(page%ESP_MAX_HTTP_CONN,path);
        check_response(page%ESP_MAX_HTTP_CONN,200,"text/html; charset=UTF-8");
        int slot=slot_for(page%ESP_MAX_HTTP_CONN);
        body((unsigned)slot,html,sizeof html);assert(!strcmp(html,lpc_test_page));
    }
    lpc_test_page=NULL;
    request(0,"/nowa12");check_response(0,503,"text/html; charset=UTF-8");
    body(0,html,sizeof html);assert(strstr(html,"brak pliku testowego"));
    const char *aliases[] = {"/set", "/Set", "/3main"};
    for (unsigned i = 0; i < sizeof aliases / sizeof aliases[0]; ++i) {
        request(0, aliases[i]); check_response(0, 200, "text/html; charset=UTF-8");
        body(0, html, sizeof html);
        assert(strstr(html, "href='temp001'"));
        assert(strstr(html, "href='temp005'"));
        assert(strstr(html, "n_GPIO_val_005"));
    }
    request(0, "/temp005"); check_response(0, 200, "text/html; charset=UTF-8");
    body(0, html, sizeof html);
    assert(strstr(html, "Temperatura Port 5"));
    assert(strstr(html, "Wilgotnosc Port 5"));
    assert(strstr(html, "n_s_Czujki_prog_max1_009"));
    assert(strstr(html, "n_s_Czujki_prog_max1_010"));
    assert(strstr(html, "action=\"/lpc.cgi/A117\""));
    request(0, "/temp001"); check_response(0, 200, "text/html; charset=UTF-8");
    body(0, html, sizeof html);
    assert(strstr(html, "Temperatura Port 1"));
    assert(!strstr(html, "Wilgotnosc Port 1"));

    const char *invalid[] = {"/temp000", "/temp021", "/temp999", "/temp015", "/temp016",
        "/temp001x", "/temp001?foo=1", "/temp01", "/tempx01", "/tempset", "/setx", "/unknown"};
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        request(0, invalid[i]); check_response(0, 404, "text/html; charset=UTF-8");
    }
    clear_queue(); HTTP_HandleTarget(0, NULL, 0, NULL);
    check_response(0, 400, "text/html; charset=UTF-8");
    request(0, "/TME.txt"); check_response(0, 200, "text/plain; charset=UTF-8");

    /* A queue slot, rather than the network channel, owns its rendered bytes. */
    clear_queue(); HTTP_HandleTarget(3, "/temp005", 8, NULL);
    assert(slot_for(3) == 0); body(0, first, sizeof first);
    HTTP_HandleTarget(1, "/temp001", 8, NULL);
    assert(slot_for(1) == 1); body(0, html, sizeof html);
    assert(strcmp(html, first) == 0);
    body(1, html, sizeof html);
    assert(strstr(html, "Temperatura Port 1"));
    assert(!strstr(html, "Temperatura Port 5"));
    assert(strcmp(html, first) != 0);
    assert(httpPar.web[0][1] != httpPar.web[1][1]);
    check_response(3, 200, "text/html; charset=UTF-8");
    check_response(1, 200, "text/html; charset=UTF-8");

    /* The real incremental +IPD decoder must reach every ESP HTTP channel. */
    for (unsigned channel = 0; channel < ESP_MAX_HTTP_CONN; ++channel) {
        clear_queue(); HTTP_RequestStreamInit(&stream, HTTP_HandleTarget, NULL);
        feed_request(&stream, channel, "/temp005", channel + 1);
        check_response(channel, 200, "text/html; charset=UTF-8");
    }
    form(target, 5, 1); clear_queue();
    HTTP_RequestStreamInit(&stream, HTTP_HandleTarget, NULL);
    feed_request(&stream, 2, target, 7);
    assert(saves == 1);
    check_response(2, 200, "text/html; charset=UTF-8");
    body(0, html, sizeof html);
    assert(strstr(html, "Zapisano ustawienia."));
    assert(strstr(html, "Salon &amp; &quot;kuchnia&quot;"));
    assert(strstr(html, "value=\"+26.1\""));
    assert(strstr(html, "value=\"+85.0\""));
    assert(strstr(html, "value=\"01:30\""));
    assert(HTTP_SensorSettingsGet(5, 0)->max1 == 261);
    assert(strstr(html, "window.history.replaceState(null,'','/temp005')"));
    assert(HTTP_SensorSettingsGet(5, 1)->max1 == 850);
    assert(HTTP_SensorSettingsInit(load, save, persisted) == 1);
    assert(HTTP_SensorSettingsGet(5, 0)->max1 == 261);

    /* Do not execute again while this connection still has a queued response. */
    HTTP_HandleTarget(2, target, strlen(target), NULL); assert(saves == 1);
    form(target, 5, 1); strcat(target, "&unexpected=1"); request(0, target);
    check_response(0, 400, "text/html; charset=UTF-8"); assert(saves == 1);
    body(0, html, sizeof html); assert(strstr(html, "Nie zapisano:"));
    form(target, 5, 1); storage_failure = 1; request(0, target);
    check_response(0, 503, "text/html; charset=UTF-8"); assert(saves == 2);
    body(0, html, sizeof html); assert(strstr(html, "Sprawdz karte SD."));
    assert(HTTP_SensorSettingsGet(5, 0)->max1 == 261);
    storage_failure = 0;
    clear_queue();
    for (unsigned channel = 0; channel < ESP_MAX_HTTP_CONN; ++channel)
        HTTP_HandleTarget(channel, "/temp001", 8, NULL);
    unsigned before = saves; HTTP_HandleTarget(0, target, strlen(target), NULL);
    HTTP_HandleTarget(ESP_MAX_HTTP_CONN, target, strlen(target), NULL);
    assert(saves == before);
    assert(reload_count > 20);
    assert(bindings_count == 0);
    HTTP_SettingsSetDefaultResolver(NULL,NULL);
    puts("PASS: /set aliases, sensor links/cards, strict routes, HTTP lengths/UTF-8, queued buffer isolation, all ESP channels and persistent CGI 200/400/503 responses.");
    return 0;
}
'''


def harness_source(tests=TESTS):
    """Extract production routing for sensor and generic-page integration checks."""
    esp = (ROOT / "Application/esp32wroom.c").read_text()
    source = (ROOT / "Application/wwwPages.c").read_text()
    header = (ROOT / "Application/wwwPages.h").read_text()
    hardware_header = (ROOT / "Application/esp32wroom.h").read_text()
    channels = re.search(r"#define\s+ESP_MAX_HTTP_CONN\s+(\d+)", hardware_header)[1]
    web_enum = re.search(r"typedef enum\s*\{\s*WWW_MAIN_SETTING,.*?\}\s*WEB_WWW\s*;", esp, re.S)[0]
    enums = "\n".join(re.findall(r"typedef\s+enum\s*\{.*?\}\s*\w+\s*;", header, re.S))
    queue = esp[esp.index("#define MAX_HTML_WEBs"):esp.index("TimerHandle_t xWaitOnSendTimeoutTimer")]
    buffers = esp[esp.index("/* Queued pages must retain"):esp.index("static char* pMem=NULL;")]
    buffers = re.sub(r"^SDRAM static HTTP_RequestStream.*?;\n", "", buffers, flags=re.M)
    route = esp[esp.index("static char* HTTP_SetWeb("):esp.index('static void HTTP_FeedReceiveRing (')]
    read_templates = source[source.index("char* HTTP_TEMPLATE_AdcTempRhu("):source.index("/* Separate tile templates for HttpMainReadPanel2.")]
    read_hidden = source[source.index('char *HTTP_TEMPLATE_Hidden2 ('):source.index("/* Mode values are CGI")]
    helpers = source[source.index("/* Mode values are CGI"):source.index("SDRAM_ALIGN32 char HttpRefr_sdram")]
    templates = source[source.index("/* Settings page templates."):source.index("const char HttpStructMainReadPanel2")]
    fragments = "\n".join(constant(source, name) for name in
                          ["HttpStyle", "HttpMainMenu", "HttpStructMainReadPanel", "HttpStructMainReadPanel2"])
    harness = (PREAMBLE + "\n#define ESP_MAX_HTTP_CONN " + channels + "\n" + web_enum + enums +
               fragments + read_templates + read_hidden + helpers + templates + queue + buffers + route + tests)
    return harness


def compile_and_run(harness):
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "A host C compiler is required"
    with tempfile.TemporaryDirectory(prefix="http-sensor-routes-") as directory:
        tmp = Path(directory)
        test = tmp / "routes.c"
        test.write_text(harness)
        executable = tmp / "routes"
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-g",
                        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"), "-I" + str(ROOT / "Application/LPC2148"),
                        "-I" + str(ROOT / "Application/General_Utilities"),
                        str(test), str(ROOT / "Application/LPC2148/lpc_manifest.c"),
                        str(ROOT / "Application/LPC2148/lpc_settings.c"),
                        str(ROOT / "Application/LPC2148/lpc_bridge.c"),
                        str(ROOT / "Application/LPC2148/http_lpc_panel.c"),
                        str(ROOT / "Application/LPC2148/http_lpc_assets.c"),
                        str(ROOT / "Application/HTTP/http_request_stream.c"),
                        str(ROOT / "Application/HTTP/http_output_control.c"),
                        str(ROOT / "Application/relay_states.c"),
                        str(ROOT / "Application/HTTP/http_sensor_page.c"),
                        str(ROOT / "Application/HTTP/http_sensor_settings.c"),
                        str(ROOT / "Application/HTTP/http_settings_catalog.c"),
                        str(ROOT / "Application/HTTP/http_settings_model.c"),
                        str(ROOT / "Application/HTTP/http_settings_page.c"),
                        str(ROOT / "Application/General_Utilities/mini_printf.c"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


def main():
    compile_and_run(harness_source())


if __name__ == "__main__":
    main()
