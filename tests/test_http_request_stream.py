#!/usr/bin/env python3
"""Exercise ESP-AT framing and HTTP request extraction on the host.

Run: python3 tests/test_http_request_stream.py
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

HARNESS = r'''
#include "http_request_stream.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned channel;
    size_t length;
    int error;
    char target[HTTP_REQUEST_STREAM_MAX_TARGET + 1u];
} Event;

static Event events[32];
static unsigned event_count;
static HTTP_RequestStream stream;
static int callback_cookie;

static void event(unsigned channel, const char *target, size_t length, void *ctx)
{
    Event *item;
    assert(ctx == &callback_cookie);
    assert(event_count < sizeof(events)/sizeof(events[0]));
    item = &events[event_count++];
    item->channel = channel;
    item->length = length;
    item->error = target == NULL;
    if (target != NULL) {
        assert(length <= HTTP_REQUEST_STREAM_MAX_TARGET);
        assert(strlen(target) == length);
        memcpy(item->target, target, length + 1u);
    } else {
        assert(length == 0u);
    }
}

static void init(void)
{
    event_count = 0u;
    memset(events, 0, sizeof(events));
    HTTP_RequestStreamInit(&stream, event, &callback_cookie);
}

static void feed(const char *bytes, size_t length)
{
    HTTP_RequestStreamFeed(&stream, (const uint8_t *)bytes, length);
}

static void text(const char *bytes)
{
    feed(bytes, strlen(bytes));
}

static void frame(unsigned channel, const char *payload, size_t length)
{
    char header[64];
    int count = snprintf(header, sizeof(header), "+IPD,%u,%lu:",
                         channel, (unsigned long)length);
    assert(count > 0 && (size_t)count < sizeof(header));
    text(header);
    feed(payload, length);
}

static void frame_text(unsigned channel, const char *payload)
{
    frame(channel, payload, strlen(payload));
}

static void valid(unsigned index, unsigned channel, const char *target)
{
    assert(index < event_count);
    assert(!events[index].error);
    assert(events[index].channel == channel);
    assert(events[index].length == strlen(target));
    assert(strcmp(events[index].target, target) == 0);
}

static void bad(unsigned index, unsigned channel)
{
    assert(index < event_count);
    assert(events[index].error && events[index].channel == channel);
}

static void split_everywhere(void)
{
    const char request[] = "GET /set?temp=1&label=%2BIPD%2C HTTP/1.1\r\nHost: esp\r\n\r\n";
    char input[512];
    size_t split, length;
    int count = snprintf(input, sizeof(input),
                         "\r\nOK\r\n0,CONNECT\r\n+IPD,0,%lu:%s\r\nSEND OK\r\n",
                         (unsigned long)strlen(request), request);
    assert(count > 0 && (size_t)count < sizeof(input));
    length = (size_t)count;
    for (split = 0u; split <= length; ++split) {
        init();
        feed(input, split);
        feed(input + split, length - split);
        assert(event_count == 1u);
        valid(0u, 0u, "/set?temp=1&label=%2BIPD%2C");
    }
    init();
    for (split = 0u; split < length; ++split) feed(input + split, 1u);
    assert(event_count == 1u);
    valid(0u, 0u, "/set?temp=1&label=%2BIPD%2C");
}

static void fragments_and_channels(void)
{
    init();
    frame_text(0u, "GET /part?");
    frame_text(1u, "GET /other HTTP/1.0\r");
    assert(event_count == 0u);
    frame_text(0u, "a=1 HTTP/1.1\r");
    frame_text(1u, "\nHost: anything\r\n\r\n");
    assert(event_count == 1u);
    valid(0u, 1u, "/other");
    frame_text(0u, "\nHost: esp\r\n\r\n");
    assert(event_count == 2u);
    valid(1u, 0u, "/part?a=1");
    frame_text(0u, "GET /must-not-repeat HTTP/1.1\r\n");
    assert(event_count == 2u);
    HTTP_RequestStreamResetChannel(&stream, 0u);
    frame_text(0u, "GET /again HTTP/1.1\r\n");
    assert(event_count == 3u);
    valid(2u, 0u, "/again");
    HTTP_RequestStreamResetChannel(&stream, 999u);
}

static void do_not_parse_payload_as_at(void)
{
    const char nested[] = "GET /nested HTTP/1.1\r\n";
    char payload[256];
    int count;
    init();
    count = snprintf(payload, sizeof(payload), "+IPD,0,%lu:%s",
                     (unsigned long)strlen(nested), nested);
    assert(count > 0 && (size_t)count < sizeof(payload));
    frame(HTTP_REQUEST_STREAM_CHANNELS, payload, (size_t)count);
    assert(event_count == 0u);
    frame_text(0u, "GET /outer HTTP/1.1\r\n");
    valid(0u, 0u, "/outer");
    frame(0u, payload, (size_t)count);
    assert(event_count == 1u);
    /* +IPD appearing in a target is ordinary data, too. */
    HTTP_RequestStreamResetChannel(&stream, 0u);
    frame_text(0u, "GET /+IPD,1,99:abc HTTP/1.1\r\n");
    assert(event_count == 2u);
    valid(1u, 0u, "/+IPD,1,99:abc");
}

static void malformed_requests(void)
{
    static const char *const requests[] = {
        "POST / HTTP/1.1\r\n",
        "get / HTTP/1.1\r\n",
        "GET http://host/set HTTP/1.1\r\n",
        "GET * HTTP/1.1\r\n",
        "GET / HTTP/2.0\r\n",
        "GET / HTTP/1.1 extra\r\n",
        "GET / HTTP/1.10\r\n",
        "GET / HTTP/1.1 \r\n",
        "GET /two words HTTP/1.1\r\n",
        "GET /fragment#no HTTP/1.1\r\n",
        "GET  HTTP/1.1\r\n",
        "GET / HTTP/1.1\n",
        "GET / HTTP/1.1\rx",
        "GET /tab\t HTTP/1.1\r\n",
        "GET /del\177 HTTP/1.1\r\n",
        "GET /raw\200 HTTP/1.1\r\n"
    };
    const char nul[] = "GET /null\0part HTTP/1.1\r\n";
    unsigned index;
    for (index = 0u; index < sizeof(requests)/sizeof(requests[0]); ++index) {
        init();
        frame(0u, requests[index], strlen(requests[index]));
        assert(event_count == 1u);
        bad(0u, 0u);
        frame_text(0u, "GET /ignored HTTP/1.1\r\n");
        assert(event_count == 1u);
        HTTP_RequestStreamResetChannel(&stream, 0u);
        frame_text(0u, "GET /reset HTTP/1.1\r\n");
        assert(event_count == 2u);
        valid(1u, 0u, "/reset");
    }
    init();
    frame(0u, nul, sizeof(nul) - 1u);
    assert(event_count == 1u);
    bad(0u, 0u);
}

static void bounds(void)
{
    static char request[HTTP_REQUEST_STREAM_MAX_TARGET + 256u];
    static char expected[HTTP_REQUEST_STREAM_MAX_TARGET + 1u];
    size_t length;
    memset(expected, 'a', sizeof(expected));
    expected[0] = '/';
    expected[sizeof(expected) - 1u] = '\0';
    length = (size_t)snprintf(request, sizeof(request),
                             "GET %s HTTP/1.1\r\n", expected);
    init();
    /* Split one long URL across many independent +IPD frames. */
    {
        size_t pos;
        for (pos = 0u; pos < length; ++pos) frame(0u, request + pos, 1u);
    }
    assert(event_count == 1u);
    valid(0u, 0u, expected);
    init();
    length = (size_t)snprintf(request, sizeof(request),
                             "GET %sx HTTP/1.1\r\n", expected);
    frame(0u, request, length);
    assert(event_count == 1u);
    bad(0u, 0u);
    init();
    memset(request, 'x', sizeof(request));
    frame(0u, request, sizeof(request));
    assert(event_count == 1u);
    bad(0u, 0u);
}

static void malformed_headers(void)
{
    static const char *const headers[] = {
        "+IPD,0,:garbage", "+IPD,0,-1:garbage", "+IPD,0,1x:garbage",
        "+IPD,0,4294967296:garbage", "+IPD,0,00000000000:garbage"
    };
    unsigned index;
    for (index = 0u; index < sizeof(headers)/sizeof(headers[0]); ++index) {
        init();
        text(headers[index]);
        assert(event_count == 1u);
        bad(0u, 0u);
        frame_text(1u, "GET /recover HTTP/1.1\r\n");
        assert(event_count == 2u);
        valid(1u, 1u, "/recover");
    }
    init();
    text("+IPD,,2:xx+IPD,x,12:garbage+IPD,4294967296,1:x+IPD,0,0:");
    assert(event_count == 0u);
    text("noise+IP+IPD,0,1:G");
    frame_text(0u, "ET /recovered HTTP/1.1\r\n");
    assert(event_count == 1u);
    valid(0u, 0u, "/recovered");
}

static void oversized_frame_is_discarded(void)
{
    static char payload[HTTP_REQUEST_STREAM_MAX_FRAME + 1u];
    init();
    memset(payload, 'x', sizeof(payload));
    memcpy(payload, "+IPD,1,16:GET / HTTP/1.1\r\n",
           sizeof("+IPD,1,16:GET / HTTP/1.1\r\n") - 1u);
    frame(0u, payload, sizeof(payload));
    assert(event_count == 1u);
    bad(0u, 0u);
    frame_text(1u, "GET /after HTTP/1.1\r\n");
    assert(event_count == 2u);
    valid(1u, 1u, "/after");
}

static void reset_and_no_callback(void)
{
    init();
    frame_text(0u, "GET /partial");
    HTTP_RequestStreamResetChannel(&stream, 0u);
    frame_text(0u, "GET /clean HTTP/1.1\r\n");
    assert(event_count == 1u);
    valid(0u, 0u, "/clean");
    HTTP_RequestStreamInit(&stream, NULL, NULL);
    frame_text(0u, "GET / HTTP/1.1\r\n");
    HTTP_RequestStreamResetChannel(&stream, 0u);
    frame_text(0u, "BAD\r\n");
    HTTP_RequestStreamFeed(&stream, NULL, 0u);
}

static void ordered_connect_and_closed(void)
{
    const char burst[] = "\r\n0,CLOSED\r\n0,CONNECT\r\n"
                         "+IPD,0,19:GET /new HTTP/1.1\r\n";
    size_t split;
    for (split = 0u; split <= sizeof(burst) - 1u; ++split) {
        init();
        frame_text(0u, "GET /old HTTP/1.1\r\n");
        frame_text(1u, "GET /partial");
        feed(burst, split);
        feed(burst + split, sizeof(burst) - 1u - split);
        assert(event_count == 2u);
        valid(0u, 0u, "/old");
        valid(1u, 0u, "/new");
        frame_text(1u, " HTTP/1.0\r\n");
        assert(event_count == 3u);
        valid(2u, 1u, "/partial");
    }
    init();
    frame_text(0u, "GET /original HTTP/1.1\r\n");
    frame_text(HTTP_REQUEST_STREAM_CHANNELS, "\r\n0,CLOSED\r\n0,CONNECT\r\n");
    frame_text(0u, "GET /must-be-ignored HTTP/1.1\r\n");
    assert(event_count == 1u);
    text("\r\n0,CONNECT\r\n");
    frame_text(0u, "GET /connected HTTP/1.1\r\n");
    assert(event_count == 2u);
    valid(1u, 0u, "/connected");
}


static void post_requests(void)
{
    const char request[]="POST /lpc.cgi/A117 HTTP/1.1\r\nHost: stm32\r\ncOnTeNt-TyPe: application/x-www-form-urlencoded; charset=UTF-8\r\nContent-Length: 13\r\n\r\nx=1&text=%2B2";
    const size_t size=sizeof(request)-1;
    for(size_t split=0;split<=size;++split){
        init(); frame(0,request,split);
        if(split<size) assert(event_count==0);
        frame(0,request+split,size-split);
        assert(event_count==1); valid(0,0,"/lpc.cgi/A117?x=1&text=%2B2");
    }
    init();
    frame_text(0,"POST /lpc.cgi/A117 HTTP/1.1\r\nContent-Length: 3\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\na=");
    assert(event_count==0);
    frame_text(1,"GET /set HTTP/1.1\r\n"); valid(0,1,"/set");
    frame_text(0,"1"); valid(1,0,"/lpc.cgi/A117?a=1");

    /* The aggregate normalized target must fit exactly, including CGI prefix. */
    {
        static char body[HTTP_REQUEST_STREAM_MAX_TARGET];
        char headers[256];
        const size_t count=HTTP_REQUEST_STREAM_MAX_TARGET-14u;
        memset(body,'a',count);
        int length=snprintf(headers,sizeof headers,"POST /lpc.cgi/A117 HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %zu\r\n\r\n",count);
        assert(length>0 && (size_t)length<sizeof headers);
        init(); frame_text(0,headers);
        for(size_t offset=0;offset<count;){
            size_t part=count-offset;if(part>100u)part=100u;
            frame(0,body+offset,part);offset+=part;
            if(offset<count)assert(event_count==0);
        }
        assert(event_count==1 && !events[0].error && events[0].length==HTTP_REQUEST_STREAM_MAX_TARGET);
        snprintf(headers,sizeof headers,"POST /lpc.cgi/A117 HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %zu\r\n\r\n",count+1u);
        init();frame_text(0,headers);assert(event_count==1);bad(0,0);
    }
    const char *invalid[]={
        "Content-Type: application/x-www-form-urlencoded\r\n\r\n",
        "Content-Length: 0\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n",
        "Content-Length: -1\r\n\r\n",
        "Content-Length: 99999999999999999999999999\r\n\r\n",
        "Content-Length: 3\r\nContent-Length: 3\r\n\r\n",
        "Content-Length: 3\r\nContent-Type: text/plain\r\n\r\n",
        "Content-Length: 3\r\n\r\n",
        "Transfer-Encoding: chunked\r\n\r\n",
        "Content-Length: 3\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\na\n1",
    };
    for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i){
        init();frame_text(0,"POST /lpc.cgi/A117 HTTP/1.1\r\n");
        assert(event_count==0);frame_text(0,invalid[i]);assert(event_count==1);bad(0,0);
    }
    init();frame_text(0,"POST /lpc.cgi/A117 HTTP/1.1\r\nContent-Length: 3\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\na=");
    text("\r\n0,CLOSED\r\n0,CONNECT\r\n");
    frame_text(0,"GET /set HTTP/1.1\r\n");assert(event_count==1);valid(0,0,"/set");
}

int main(void)
{
    post_requests();
    split_everywhere();
    fragments_and_channels();
    do_not_parse_payload_as_at();
    malformed_requests();
    bounds();
    malformed_headers();
    oversized_frame_is_discarded();
    reset_and_no_callback();
    ordered_connect_and_closed();
    puts("HTTP request stream: framing, fragments, limits and reset checks passed");
    return 0;
}
'''


def main():
    compiler = shutil.which("gcc") or shutil.which("cc")
    if compiler is None:
        raise RuntimeError("A C compiler is required")
    with tempfile.TemporaryDirectory(prefix="http-stream-") as directory:
        directory = Path(directory)
        harness = directory / "harness.c"
        harness.write_text(HARNESS)
        for configuration in ([], ["-DHTTP_REQUEST_STREAM_CHANNELS=3",
                                    "-DHTTP_REQUEST_STREAM_MAX_TARGET=128",
                                    "-DHTTP_REQUEST_STREAM_MAX_FRAME=512"]):
            executable = directory / "test"
            subprocess.run([
                compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-g",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                *configuration, "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"),
                str(harness), str(ROOT / "Application/HTTP/http_request_stream.c"),
                "-o", str(executable),
            ], check=True)
            environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
            subprocess.run([str(executable)], env=environment, check=True)


if __name__ == "__main__":
    main()
