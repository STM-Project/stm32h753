#include "google_cloud_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    CloudHttpResponse r;
    const char *nonce="0123456789abcdef0123456789abcdef";
    const char *body="STM32CLOUD1:0123456789abcdef0123456789abcdef:1";
    char message[1024], path[2048];
    int relay=-1;
    int len=snprintf(message,sizeof(message),"HTTP/1.1 200 OK\r\nContent-Length: %u\r\n\r\n%s",(unsigned)strlen(body),body);
    for (int n=0;n<len;++n) assert(CloudHttp_Parse(message,(size_t)n,0,&r)==1);
    assert(CloudHttp_Parse(message,(size_t)len,0,&r)==0);
    assert(CloudReply_Parse(r.body,nonce,&relay)==0 && relay==1);
    assert(CloudReply_Parse(r.body,"1123456789abcdef0123456789abcdef",&relay)==-1);
    assert(CloudReply_Parse("ERROR:request",nonce,&relay)==-1);
    assert(CloudReply_Parse("STM32CLOUD1:0123456789abcdef0123456789abcdef:1junk",nonce,&relay)==-1);
    len=snprintf(message,sizeof(message),"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n%x;foo=bar\r\n%s\r\n0\r\n\r\n",(unsigned)strlen(body),body);
    for (int n=0;n<len;++n) assert(CloudHttp_Parse(message,(size_t)n,0,&r)==1);
    assert(CloudHttp_Parse(message,(size_t)len,0,&r)==0 && strcmp(r.body,body)==0);
    assert(CloudHttp_Parse(message,(size_t)len-3,1,&r)==-1);
    const char *bad="HTTP/1.1 200 OK\r\nContent-Length: 1\r\nTransfer-Encoding: chunked\r\n\r\n";
    assert(CloudHttp_Parse(bad,strlen(bad),1,&r)==-1);
    bad="HTTP/1.1 200 OK\r\nContent-Length: 9999999999999999999999\r\n\r\n";
    assert(CloudHttp_Parse(bad,strlen(bad),1,&r)==-1);
    bad="HTTP/1.1 200 OK\r\nContent-Length: 999\r\n\r\n";
    assert(CloudHttp_Parse(bad,strlen(bad),1,&r)==-1);
    bad="HTTP/1.1 403 Forbidden\r\n\r\nERROR";
    assert(CloudHttp_Parse(bad,strlen(bad),1,&r)==-1);
    bad="HTTP/1.1 303 See Other\r\nLocation: https://script.googleusercontent.com/macros/echo?x=1\r\n\r\n";
    assert(CloudHttp_Parse(bad,strlen(bad),0,&r)==0 && r.status==303);
    assert(CloudRedirect_Path(r.location,path,sizeof(path))==0 && !strcmp(path,"/macros/echo?x=1"));
    assert(CloudRedirect_Path("http://script.googleusercontent.com/x",path,sizeof(path))==-1);
    assert(CloudRedirect_Path("https://script.googleusercontent.com.evil.test/x",path,sizeof(path))==-1);
    assert(CloudRedirect_Path("https://script.googleusercontent.com/x\r\nInjected: 1",path,sizeof(path))==-1);
    assert(CloudRedirect_Path("https://evil.test/x",path,sizeof(path))==-1);
    assert(CloudRedirect_Path("https://script.googleusercontent.com/x",path,2)==-1);
    puts("HTTP framing, partial reads, redirects, nonce and relay validation: PASS");
}
