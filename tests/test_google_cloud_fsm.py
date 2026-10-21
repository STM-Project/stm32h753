#!/usr/bin/env python3
"""Drive the production vtaskWifi Google case with fragmented ESP replies."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'Application/esp32wroom.c').read_text()
def function(start):
 a=s.index(start)
 if start=='static const char* CLOUD_Host':return s[a:s.index('\n',a)]
 return s[a:s.index('\n}',a)+2]
case=s[s.index('                case GOOGLE_CLOUD_CONNECTION:'):s.index('\n\t\t\t\tcase HTTP_CONNECTION_2:')]
helpers='\n'.join(function(t) for t in ['static const char* CLOUD_Host','__attribute__((weak)) int GoogleCloud_ReadTemperature10','static void CLOUD_Close','static void CLOUD_RestoreHttp','static void CLOUD_FeedReceiveRing','static TickType_t CLOUD_WaitTicks'])
C=r'''
#include "google_cloud.h"
#include "google_cloud_at.h"
#include "google_cloud_protocol.h"
#include "google_cloud_config.h"
#include "google_cloud_roots.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define _OK 1
#define _ERROR 2
#define HAL_OK 0
#define DBG 0
#define BIT_ESP_SRV 1
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY UINT32_MAX
#define WIFI_MODE_STA 1
#define WIFI_MODE_AP_STA 3
static struct {struct {int mode;} wifiGeneral;} Const;
#define ESP_OFF ((void)0)
#define ESP_ON ((void)0)
#define _GET_ACTUAL_CASE_ CASE_Service(-1,NULL,NULL,0)
#define _GET_ANSW_CASE_ CASE_Service(-5,NULL,NULL,0)
#define _CLR_REP_CASE_ CASE_Service(-4,NULL,NULL,0)
#define _SET_NEW_CASE_(n) CASE_Service(n,NULL,NULL,0);_CLR_REP_CASE_
#define _SET_NEXT_CASE_ CASE_Service(-2,NULL,NULL,0)
#define DbgVarDma(...) ((void)0)
#define noArch 0
#define mini_snprintf snprintf
#define RecvFromEsp(s) NULL
typedef int ARCHIVING_TYPE;
enum{HTTP_CONNECTION,GOOGLE_CLOUD_CONNECTION,INIT_CONNECTION};
static int connectionType;
volatile GoogleCloudStatus googleCloudStatus;
static CloudAt cloudAt;static CloudHttpResponse cloudResponse;
static char cloudRequest[4096],cloudRedirect[2048],cloudNonce[33],sendBuff[2048];
static size_t cloudRequestLength,cloudOffset,cloudPacketLength;
static uint32_t cloudCleanupTick,cloudLastAttempt,cloudStarted,ticks;
static size_t cloudCertOffset;
static struct{int gState;} ESP_UART_HANDLE;
#define HAL_UART_STATE_READY 0
#define _SET 1
static void COMMAND_Service(int n,char*p){(void)n;(void)p;}
static int cloudRedirected,cloudResult;
static char HttpRefr_sdram[320];
static char RecvBuffer[4096];static int read_pos,recvByteFromEsp_copy;
static void* vtaskWifiHandle;static int httpRequestStream;
static const char *txt_OK="\r\nOK\r\n",*txt_ERR="ERROR";
static char sent[4096],payload[8192];static size_t sent_length,payload_used;static unsigned commands;
static void UpdateReadPos(void){read_pos=recvByteFromEsp_copy;}
static void IncPos(int *pos){*pos=(*pos+1)%4096;}
static void InitStructRqstToSendChnl(void){}
static void HTTP_HandleTarget(void){}
static void HTTP_RequestStreamInit(void*p,void(*cb)(void),void*c){(void)p;(void)cb;(void)c;}
static void HTTP_RequestStreamFeed(void*p,const uint8_t*b,size_t n){(void)p;(void)b;(void)n;}
static unsigned xTaskGetTickCount(void){return ticks;}
static void xTaskNotify(void*p,unsigned b,int a){(void)p;(void)b;(void)a;}
#define eSetBits 0
static void vTaskDelay(unsigned n){(void)n;}
static void RcvTimeoutTimerStop(void){}
static void DispRecvBuff(int n,int a){(void)n;(void)a;}
static void ESP32_FreeAnswers(int n){(void)n;}
static const char *raw="";
static char *strstr_(void*p,const char*s){(void)p;return strstr((char*)raw,s);}
static int GetHttpPort(void){return 80;}
static int SendToEsp32(int length,char*data,int arch){
 (void)arch;if(!data)data=sendBuff;if(!length)length=(int)strlen(data);
 assert(length>0&&length<2048);memcpy(sent,data,(size_t)length);sent[length]=0;sent_length=(size_t)length;commands++;
 if(strncmp(sent,"AT+",3)){assert(payload_used+(size_t)length<sizeof payload);memcpy(payload+payload_used,sent,(size_t)length);payload_used+=(size_t)length;payload[payload_used]=0;}
 return HAL_OK;
}
'''
C+=function('static int CASE_Service(int nrCase,')+'\nstatic void BackToHttpService(int*p){(void)p;connectionType=HTTP_CONNECTION;_SET_NEW_CASE_(0);UpdateReadPos();}\n'+helpers
init_at=s.index('                    else if (CASE_Service(22,">",txt_ERR,noArch))')
init=s[init_at:s.index('\n\t\t\t\t\telse\n',init_at)]
init=init.replace('else if (CASE_Service(22,','if (CASE_Service(22,',1)
C+='\nstatic void init_step(const char *data){raw=data;'+init+'}\n'
a=s.index('        if(connectionType==INIT_CONNECTION && googleCloudStatus.configured &&')
b=s.index('        }else if(connectionType==HTTP_CONNECTION && googleCloudStatus.ready &&',a)
C+='\nstatic void periodic(void){int nrHTTPpacket=0;(void)nrHTTPpacket;'+s[a:b]+'}}\n'
C+='\nstatic void step(const char *data){int nrHTTPpacket=0;for(;*data;data++){RecvBuffer[recvByteFromEsp_copy]=*data;recvByteFromEsp_copy=(recvByteFromEsp_copy+1)%4096;}switch(connectionType){'+case+'default:break;}}\n'
C+=r'''
static const char *token="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static void begin(void){
 memset((void*)&googleCloudStatus,0,sizeof googleCloudStatus);CloudAt_Reset(&cloudAt);cloudRedirected=0;payload_used=0;read_pos=0;recvByteFromEsp_copy=0;
 strcpy(cloudNonce,"0123456789abcdef0123456789abcdef");
 cloudRequestLength=CloudRequest_Build(cloudRequest,sizeof cloudRequest,"script.google.com","/macros/s/test-deployment/exec",token,"stm32h753",251,0,cloudNonce);
 assert(cloudRequestLength);connectionType=GOOGLE_CLOUD_CONNECTION;_SET_NEW_CASE_(0);
}
static void connect_and_send(void){
 step("\r\nOK\r\n");assert(strstr(sent,"CIPSSLCCN=3"));
 step("\r\nOK\r\n");assert(strstr(sent,"CIPSSLCSNI=3"));
 step("\r\nOK\r\n");assert(strstr(sent,"CIPSTART=3,\"SSL\""));
 step("3,CONNECT\r\nOK\r\n");assert(strstr(sent,"CIPSEND=3,"));
 step(">");assert(strstr(sent,"POST /macros/s/"));assert(strstr(sent,token));
}
static void receive(const char*http){
 char frame[9000];snprintf(frame,sizeof frame,"\r\nSEND OK\r\n+IPD,3,%u:%s",(unsigned)strlen(http),http);
 for(size_t i=0;i<strlen(frame);i++){char one[]={frame[i],0};step(one);}
}
int main(void){
 /* Idle cloud must not poll every 100ms; retain deadlines and active timeouts. */
 connectionType=HTTP_CONNECTION;googleCloudStatus.configured=1;googleCloudStatus.ready=1;
 Const.wifiGeneral.mode=WIFI_MODE_STA;cloudLastAttempt=1000;ticks=1000;
 assert(CLOUD_WaitTicks()==GOOGLE_CLOUD_PERIOD_MS);
 ticks=1100;assert(CLOUD_WaitTicks()==GOOGLE_CLOUD_PERIOD_MS-100);
 ticks=1000+GOOGLE_CLOUD_PERIOD_MS;assert(CLOUD_WaitTicks()==250);
 cloudLastAttempt=UINT32_MAX-49;ticks=50;
 assert(CLOUD_WaitTicks()==GOOGLE_CLOUD_PERIOD_MS-100);
 googleCloudStatus.ready=0;assert(CLOUD_WaitTicks()==portMAX_DELAY);
 googleCloudStatus.ready=1;Const.wifiGeneral.mode=2;assert(CLOUD_WaitTicks()==portMAX_DELAY);
 connectionType=INIT_CONNECTION;_SET_NEW_CASE_(21);assert(CLOUD_WaitTicks()==portMAX_DELAY);
 _SET_NEW_CASE_(22);assert(CLOUD_WaitTicks()==100);
 _SET_NEW_CASE_(23);assert(CLOUD_WaitTicks()==100);
 _SET_NEW_CASE_(27);assert(CLOUD_WaitTicks()==100);
 googleCloudStatus.configured=0;assert(CLOUD_WaitTicks()==portMAX_DELAY);
 connectionType=GOOGLE_CLOUD_CONNECTION;assert(CLOUD_WaitTicks()==100);
 memset(HttpRefr_sdram,' ',sizeof HttpRefr_sdram-1);memcpy(HttpRefr_sdram+73," 25.1",5);assert(GoogleCloud_ReadTemperature10()==251);
 memcpy(HttpRefr_sdram+73,"-12.3",5);assert(GoogleCloud_ReadTemperature10()==-123);memcpy(HttpRefr_sdram+73," --.-",5);assert(GoogleCloud_ReadTemperature10()==-32768);
 char http[4096],body[128];snprintf(body,sizeof body,"STM32CLOUD1:%s:1","0123456789abcdef0123456789abcdef");
 snprintf(http,sizeof http,"HTTP/1.1 200 OK\r\nContent-Length: %u\r\n\r\n%s",(unsigned)strlen(body),body);
 begin();connect_and_send();receive(http);assert(strstr(sent,"CIPCLOSE=3"));assert(googleCloudStatus.demo_relay==1);
 step("\r\nERROR\r\n");assert(strstr(sent,"CIPSERVER=1,80"));step("\r\nOK\r\n");assert(connectionType==HTTP_CONNECTION);assert(googleCloudStatus.successes==1);
 begin();connect_and_send();receive("HTTP/1.1 303 See Other\r\nLocation: https://script.googleusercontent.com/macros/echo?key=abc\r\n\r\n");
 assert(cloudRedirected&&strstr(sent,"CIPCLOSE=3"));step("\r\nOK\r\n");assert(strstr(sent,"script.googleusercontent.com"));
 step("\r\nOK\r\n");step("\r\nOK\r\n");step("\r\nOK\r\n");assert(strstr(sent,"CIPSEND"));step(">");assert(strstr(sent,"GET /macros/echo?key=abc"));assert(!strstr(sent,token));receive(http);
 step("\r\nERROR\r\n");step("\r\nOK\r\n");assert(connectionType==HTTP_CONNECTION&&googleCloudStatus.successes==1);
 begin();connect_and_send();receive("HTTP/1.1 303 See Other\r\nLocation: https://evil.test/collect\r\n\r\n");assert(!cloudRedirected);assert(strstr(sent,"CIPCLOSE"));
 step("\r\nERROR\r\n");step("\r\nOK\r\n");assert(googleCloudStatus.failures==1&&!googleCloudStatus.demo_relay);
 begin();connect_and_send();
 char long_location[2048],redirect_reply[2300];strcpy(long_location,"https://script.googleusercontent.com/macros/echo?key=");
 size_t prefix=strlen(long_location);memset(long_location+prefix,'x',2035-prefix);long_location[2035]=0;
 snprintf(redirect_reply,sizeof redirect_reply,"HTTP/1.1 303 See Other\r\nLocation: %s\r\n\r\n",long_location);receive(redirect_reply);
 step("\r\nERROR\r\n");step("\r\nOK\r\n");step("\r\nOK\r\n");step("\r\nOK\r\n");
 assert(cloudRequestLength>2047);assert(strstr(sent,"CIPSEND=3,2047"));size_t get_start=payload_used;step(">");assert(sent_length==2047);
 step("\r\nSEND OK\r\n");assert(strstr(sent,"CIPSEND=3,"));step(">");assert(sent_length==cloudRequestLength-2047);assert(!strstr(payload+get_start,token));receive(http);
 step("\r\nERROR\r\n");step("\r\nOK\r\n");assert(googleCloudStatus.successes==1);
 begin();step("\r\nERROR\r\n");assert(strstr(sent,"CIPCLOSE"));step("\r\nERROR\r\n");step("\r\nOK\r\n");assert(googleCloudStatus.last_error==-5&&connectionType==HTTP_CONNECTION);
 begin();connect_and_send();receive("HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\n1");assert(!googleCloudStatus.demo_relay);assert(strstr(sent,"CIPCLOSE"));
 begin();ticks=30000;cloudStarted=0;periodic();assert(_GET_ACTUAL_CASE_==900&&cloudResult==-4);
 ticks+=5000;periodic();assert(_GET_ACTUAL_CASE_==901&&strstr(sent,"CIPSERVER"));
 ticks+=5000;periodic();assert(connectionType==INIT_CONNECTION&&googleCloudStatus.last_error==-9);
 ticks=100;cloudCleanupTick=100;cloudCertOffset=0;payload_used=0;
 memset((void*)&googleCloudStatus,0,sizeof googleCloudStatus);googleCloudStatus.configured=1;connectionType=INIT_CONNECTION;_SET_NEW_CASE_(22);
 init_step(">");assert(_GET_ACTUAL_CASE_==23);periodic();assert(cloudCertOffset==2047);
 ESP_UART_HANDLE.gState=1;periodic();assert(cloudCertOffset==2047);ESP_UART_HANDLE.gState=HAL_UART_STATE_READY;
 while(_GET_ACTUAL_CASE_==23){periodic();}assert(_GET_ACTUAL_CASE_==24);assert(payload_used==sizeof google_cloud_roots);assert(!memcmp(payload,google_cloud_roots,sizeof google_cloud_roots));
 init_step("\r\nOK\r\n");assert(strstr(sent,"CIPSSLCCONF=3,2"));init_step("\r\nOK\r\n");assert(strstr(sent,"CIPRECVMODE=0"));init_step("\r\nOK\r\n");assert(strstr(sent,"CIPDINFO=0"));init_step("\r\nOK\r\n");assert(connectionType==HTTP_CONNECTION&&googleCloudStatus.ready);
 connectionType=INIT_CONNECTION;googleCloudStatus.ready=0;_SET_NEW_CASE_(22);init_step("\r\nERROR\r\n");assert(_GET_ACTUAL_CASE_==26);init_step("\r\nOK\r\n");init_step("\r\nOK\r\n");assert(connectionType==HTTP_CONNECTION&&!googleCloudStatus.ready);
 connectionType=INIT_CONNECTION;googleCloudStatus.configured=1;_SET_NEW_CASE_(22);ticks=30000;cloudCleanupTick=0;periodic();assert(_GET_ACTUAL_CASE_==0&&!googleCloudStatus.configured);
 puts("PASS: production Google CASE_Service/SendToEsp32 FSM: SDRAM temperature, CA/DMA INIT, TLS host/SNI, fragmented replies, multi-packet redirect GET without token, errors/timeouts and WWW restore.");
}
'''
with tempfile.TemporaryDirectory() as name:
 p=Path(name);(p/'test.c').write_text(C)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(ROOT/'Application/GOOGLE'),str(p/'test.c'),str(ROOT/'Application/GOOGLE/google_cloud_at.c'),str(ROOT/'Application/GOOGLE/google_cloud_protocol.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
