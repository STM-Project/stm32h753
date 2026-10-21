#include "google_cloud_at.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
void CloudAt_Reset(CloudAt *at){memset(at,0,sizeof *at);}
static void line_done(CloudAt *at)
{
    at->line[at->line_used]=0;
    if(!at->line_overflow){
        if(!strcmp(at->line,"OK"))at->events|=CLOUD_AT_OK;
        else if(!strcmp(at->line,"ERROR")||!strcmp(at->line,"FAIL")||!strcmp(at->line,"SEND FAIL"))at->events|=CLOUD_AT_ERROR;
        else if(!strcmp(at->line,"SEND OK"))at->events|=CLOUD_AT_SENT;
        else if(!strcmp(at->line,"3,CLOSED")){at->events|=CLOUD_AT_CLOSED;at->eof=1;}
    }
    at->line_used=0;at->line_overflow=0;
}
void CloudAt_Feed(CloudAt *at,const uint8_t *data,size_t size)
{
    for(size_t i=0;i<size;i++){
        unsigned char c=data[i];
        if(at->payload){
            if(at->frame_channel==CLOUD_AT_LINK_ID){
                if(at->used+1>=sizeof at->wire)at->overflow=1;
                else{at->wire[at->used++]=(char)c;at->wire[at->used]=0;}
            }
            if(!--at->remaining)at->payload=0;
            continue;
        }
        if(c=='>' && !at->line_used){at->events|=CLOUD_AT_PROMPT;continue;}
        if(c=='\r')continue;
        if(c=='\n'){line_done(at);continue;}
        if(c==':' && at->line_used>=5 && !memcmp(at->line,"+IPD,",5)){
            at->line[at->line_used]=0;
            const char *p=at->line+5;uint32_t values[2]={0,0};int valid=1;
            for(unsigned n=0;n<2;n++){
                if(!isdigit((unsigned char)*p)){valid=0;break;}
                while(isdigit((unsigned char)*p)){
                    unsigned digit=(unsigned)(*p++-'0');
                    if(values[n]>(65535u-digit)/10u){valid=0;break;}
                    values[n]=values[n]*10u+digit;
                }
                if(!valid || (n==0?*p++!=',':*p!=0)){valid=0;break;}
            }
            if(!valid || at->line_overflow || values[0]>4 || !values[1]){at->overflow=1;at->events|=CLOUD_AT_ERROR;}
            else{at->frame_channel=values[0];at->remaining=values[1];at->payload=1;}
            at->line_used=0;at->line_overflow=0;continue;
        }
        if(at->line_used+1<sizeof at->line)at->line[at->line_used++]=(char)c;
        else at->line_overflow=1;
    }
}
int CloudAt_Has(const CloudAt *at,const char *answer)
{
    if(!answer)return 0;
    if(!strcmp(answer,"\r\nOK\r\n"))return !!(at->events&CLOUD_AT_OK);
    if(!strcmp(answer,"ERROR"))return !!(at->events&CLOUD_AT_ERROR);
    if(!strcmp(answer,">"))return !!(at->events&CLOUD_AT_PROMPT);
    if(!strcmp(answer,"\r\nSEND OK"))return !!(at->events&CLOUD_AT_SENT);
    if(!strcmp(answer,"3,CLOSED"))return !!(at->events&CLOUD_AT_CLOSED);
    return 0;
}
int CloudConfig_Valid(const char *token,const char *device,const char *path)
{
    if(!token||!device||!path||strlen(token)!=64||!strlen(device)||strlen(device)>32||strlen(path)<16||strlen(path)>512)return 0;
    for(const char *p=token;*p;p++)if(!isxdigit((unsigned char)*p))return 0;
    for(const char *p=device;*p;p++)if(!isalnum((unsigned char)*p)&&*p!='_'&&*p!='-')return 0;
    if(strncmp(path,"/macros/s/",9)||strcmp(path+strlen(path)-5,"/exec"))return 0;
    for(const char *p=path;*p;p++)if(!isalnum((unsigned char)*p)&&*p!='_'&&*p!='-'&&*p!='/')return 0;
    return 1;
}
size_t CloudRequest_Build(char *out,size_t capacity,const char *host,const char *path,const char *token,const char *device,int temperature10,int relay,const char *nonce)
{
    char body[256];int count;
    if(!out||!capacity||!host||!path||path[0]!='/')return 0;
    out[0]=0;
    if(strcmp(host,"script.google.com")&&strcmp(host,"script.googleusercontent.com"))return 0;
    for(const char *p=path;*p;p++)if((unsigned char)*p<=32||(unsigned char)*p>=127||*p=='#'||*p=='\\')return 0;
    if(token){
        if(strcmp(host,"script.google.com")||!CloudConfig_Valid(token,device,path)||!nonce||strlen(nonce)!=32||temperature10< -800||temperature10>1500||(relay!=0&&relay!=1))return 0;
        for(const char *p=nonce;*p;p++)if(!isxdigit((unsigned char)*p))return 0;
        count=snprintf(body,sizeof body,"token=%s&device=%s&temperature10=%d&relayAck=%d&nonce=%s",token,device,temperature10,relay,nonce);
        if(count<0||(size_t)count>=sizeof body)return 0;
        count=snprintf(out,capacity,"POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %u\r\nConnection: close\r\nAccept-Encoding: identity\r\n\r\n%s",path,host,(unsigned)strlen(body),body);
    }else count=snprintf(out,capacity,"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept-Encoding: identity\r\n\r\n",path,host);
    if(count<0||(size_t)count>=capacity){out[0]=0;return 0;}
    return (size_t)count;
}
