#include "google_cloud_at.h"
#include "google_cloud_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static CloudAt at;
static void feed(const char *s){CloudAt_Feed(&at,(const uint8_t*)s,strlen(s));}
int main(void){
 const char *token="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
 const char *nonce="0123456789abcdef0123456789abcdef";
 const char *path="/macros/s/deployment-test/exec";
 char request[4096],message[9000];
 assert(CloudConfig_Valid(token,"stm32h753",path));
 assert(!CloudConfig_Valid(token,"bad&device",path));
 assert(!CloudConfig_Valid(token,"stm32h753","/macros/s/id\r\n/exec"));
 size_t n=CloudRequest_Build(request,sizeof request,"script.google.com",path,token,"stm32h753",251,0,nonce);
 assert(n==strlen(request));assert(strstr(request,"POST /macros/"));
 assert(strstr(request,"temperature10=251&relayAck=0&nonce="));
 char *body=strstr(request,"\r\n\r\n")+4;char length[64];snprintf(length,sizeof length,"Content-Length: %u",(unsigned)strlen(body));assert(strstr(request,length));
 assert(!CloudRequest_Build(request,20,"script.google.com",path,token,"stm32h753",251,0,nonce));assert(!request[0]);
 assert(!CloudRequest_Build(request,sizeof request,"evil.test",path,token,"stm32h753",251,0,nonce));
 assert(CloudRequest_Build(request,sizeof request,"script.googleusercontent.com","/macros/echo?user_content_key=abcdef",NULL,NULL,0,0,NULL));
 assert(!strstr(request,token));assert(!strstr(request,"POST"));
 CloudAt_Reset(&at);
 const char *payload="HTTP/1.1 200 OK\r\nContent-Length: 23\r\n\r\nERROR\r\nOK\r\nSEND OK\r\n>";
 snprintf(message,sizeof message,"+IPD,3,%u:%s",(unsigned)strlen(payload),payload);
 for(size_t i=0;i<strlen(message);i++)CloudAt_Feed(&at,(const uint8_t*)message+i,1);
 assert(!strcmp(at.wire,payload));assert(!at.events);assert(!at.overflow);
 feed("\r\nSEND OK\r\n3,CLOSED\r\n");assert(at.events&CLOUD_AT_SENT);assert(at.eof);
 assert(CloudAt_Has(&at,"\r\nSEND OK"));
 CloudAt_Reset(&at);feed("+IPD,4,5:ERROR\r\nOK\r\n>");assert(!at.used);assert(CloudAt_Has(&at,"\r\nOK\r\n"));assert(CloudAt_Has(&at,">"));assert(!CloudAt_Has(&at,"ERROR"));
 CloudAt_Reset(&at);feed("+IPD,3,999999999999:");assert(at.overflow);
 CloudAt_Reset(&at);snprintf(message,sizeof message,"+IPD,3,%u:",CLOUD_AT_WIRE_SIZE+1u);feed(message);
 memset(message,'X',CLOUD_AT_WIRE_SIZE+1u);CloudAt_Feed(&at,(uint8_t*)message,CLOUD_AT_WIRE_SIZE+1u);assert(at.overflow);assert(at.used==CLOUD_AT_WIRE_SIZE-1u);
 puts("PASS: Google request framing, strict config, token-free redirect, fragmented IPD, opaque AT-like bodies, other channels and receive bounds.");
}
