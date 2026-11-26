"""Execute the Ethernet HTTP handler with fragmented I/O and modeled SD files."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
headers={
'lwip/api.h':r'''
#include <stddef.h>
typedef int err_t;
struct netconn {int unused;};struct netbuf {int unused;};
#define ERR_OK 0
#define ERR_WOULDBLOCK (-7)
#define ERR_TIMEOUT (-3)
#define NETCONN_TCP 1
#define NETCONN_COPY 1
#define IP_ADDR_ANY 0
struct netconn *netconn_new(int);
int netconn_recv(struct netconn*,struct netbuf**);
void netconn_set_nonblocking(struct netconn*,int);
unsigned short netbuf_len(struct netbuf*);
unsigned short netbuf_copy_partial(struct netbuf*,void*,unsigned short,unsigned short);
void netbuf_delete(struct netbuf*);
int netconn_write_partly(struct netconn*,const void*,size_t,int,size_t*);
int netconn_bind(struct netconn*,void*,unsigned short);
int netconn_listen(struct netconn*);
int netconn_delete(struct netconn*);
int netconn_accept(struct netconn*,struct netconn**);
int netconn_close(struct netconn*);
void netconn_set_recvtimeout(struct netconn*,int);
void netconn_set_sendtimeout(struct netconn*,int);
''',
'lwip/sys.h':r'''
#define osPriorityBelowNormal 1
#define osWaitForever 0xffffffffU
void *sys_thread_new(const char*,void (*)(void*),void*,int,int);
void sys_msleep(unsigned);
unsigned sys_now(void);
void vTaskDelete(void*);
''',
'ff.h':r'''
typedef unsigned UINT;typedef int FRESULT;
typedef struct {unsigned position,size;} FIL;
#define FR_OK 0
#define FA_READ 1
#define f_size(f) ((f)->size)
FRESULT f_open(FIL*,const char*,int);
FRESULT f_read(FIL*,void*,UINT,UINT*);
FRESULT f_close(FIL*);
''',
 'timer.h':'#define Semphr_cardSD 2\nint TakeMutex(int,unsigned);\nvoid GiveMutex(int);\n',
 '_debug.h':'void DbgDma(int,const char*);\n',
}
code=r'''
#include "network_http.c"
#include <assert.h>
#include <stdlib.h>
static const char *request;static size_t offset,outputSize;
static char output[220000];static unsigned fileSize,readCount,closes;
static int locked,missing,busy,writeFailure;
int TakeMutex(int id,unsigned wait){assert(id==2);if(busy&&wait!=osWaitForever)return 0;assert(!locked);locked=1;return 1;}
void GiveMutex(int id){assert(id==2&&locked);locked=0;}
int f_open(FIL *f,const char *name,int flags){assert(locked&&flags==FA_READ&&!strcmp(name,"0:/aaa.htm"));f->position=0;f->size=fileSize;return missing?1:FR_OK;}
int f_read(FIL *f,void *data,UINT size,UINT *count){assert(locked);*count=f->size-f->position;if(*count>size)*count=size;for(unsigned i=0;i<*count;i++)((char*)data)[i]=(char)('a'+(f->position+i)%26);f->position+=*count;readCount++;return FR_OK;}
int f_close(FIL *f){(void)f;assert(locked);closes++;return FR_OK;}
static int read_fragment(void *context,unsigned char *data,size_t capacity){(void)context;assert(!locked);size_t n=strlen(request)-offset;if(n>7)n=7;if(n>capacity)n=capacity;memcpy(data,request+offset,n);offset+=n;return (int)n;}
static int write_fragment(void *context,const unsigned char *data,size_t size){(void)context;assert(!locked);if(writeFailure)return -1;if(size>13)size=13;assert(outputSize+size<sizeof output);memcpy(output+outputSize,data,size);outputSize+=size;output[outputSize]=0;return (int)size;}
static int serve(const char *text){request=text;offset=outputSize=readCount=closes=0;return NetworkHTTPServe(NULL,read_fragment,write_fragment,"TLS-test");}
static const char *sharedTarget, *sharedResponse;
static int sharedReads, finished, webLocks;
int NetworkWebLock(void){++webLocks;return 1;}
void NetworkWebUnlock(void){}
int NetworkWebStart(const char *target,size_t length){
 assert(length==strlen(target)&&sharedTarget&&!strcmp(target,sharedTarget));
 sharedReads=0;finished=-1;return 1;
}
int NetworkWebRead(unsigned char *out,size_t capacity){
 if(sharedReads++)return 0;
 size_t n=strlen(sharedResponse);assert(n<=capacity);memcpy(out,sharedResponse,n);return (int)n;
}
void NetworkWebFinish(int delivered){finished=delivered;}
static int writeError;
int netconn_write_partly(struct netconn *conn,const void *data,size_t size,int flags,size_t *written){
 assert(conn&&data&&size==4&&flags==NETCONN_COPY&&written);
 if(writeError)return writeError;
 *written=2;
 return ERR_OK;
}
static unsigned now, deleted;
static struct netbuf firstRequest;
unsigned sys_now(void){return now;}
int netconn_recv(struct netconn *conn,struct netbuf **out){
 if(!conn->unused)return ERR_WOULDBLOCK;
 *out=&firstRequest;
 return ERR_OK;
}
int netconn_delete(struct netconn *conn){assert(conn);++deleted;return ERR_OK;}
void DbgDma(int on,const char *text){assert(on&&text);}
void DbgVarDma(int on,unsigned capacity,const char *format,...){assert(on&&capacity&&format);}
int main(void){
 struct netconn idle={0}, active={1};
 struct PendingConnection pending[2]={{&idle,0},{&active,0}};
 struct PlainConnection ready;
 assert(!http_take_ready(&pending[0],&ready)&&pending[0].conn==&idle&&deleted==0);
 assert(http_take_ready(&pending[1],&ready)&&ready.conn==&active&&ready.buf==&firstRequest&&!pending[1].conn);
 int previousLocks=webLocks;
 request="";offset=0;
 assert(!NetworkHTTPServe(NULL,read_fragment,write_fragment,NULL)&&webLocks==previousLocks);
 now=HTTP_IDLE_TIMEOUT_MS;
 assert(!http_take_ready(&pending[0],&ready)&&!pending[0].conn&&deleted==1);
 struct netconn client={0};
 struct PlainConnection connection={.conn=&client};
 assert(plain_write(&connection,(const unsigned char *)"test",4)==2);
 writeError=-6;
 assert(plain_write(&connection,(const unsigned char *)"test",4)==-1);
 assert(serve("GET /test HTTP/1.1\r\nHost: board\r\n\r\n"));assert(strstr(output,"200 OK")&&strstr(output,"TLS-test")&&!closes);
 request="GET /test HTTP/1.1\r\nHost: board\r\n\r\n";offset=outputSize=0;
 assert(NetworkHTTPServe(NULL,read_fragment,write_fragment,NULL));
 const char *testBody="<html><body>Test: HTTP</body></html>";
 assert(strstr(output,"HTTP/1.1 200 OK\r\n") == output);
 assert(!strcmp(strstr(output,"\r\n\r\n")+4,testBody));
 char lengthHeader[40];snprintf(lengthHeader,sizeof lengthHeader,"Content-Length: %zu\r\n",strlen(testBody));
 assert(strstr(output,lengthHeader));
 fileSize=200000;assert(serve("GET /aaa.htm HTTP/1.1\r\n\r\n")&&closes==1&&readCount>90);
 char *body=strstr(output,"\r\n\r\n")+4;assert(strlen(body)==fileSize);for(unsigned i=0;i<fileSize;i++)assert(body[i]=='a'+i%26);
 missing=1;assert(serve("GET /aaa.htm HTTP/1.1\r\n\r\n")&&strstr(output,"503 Service Unavailable")&&!closes);missing=0;
 busy=1;assert(serve("GET /aaa.htm HTTP/1.1\r\n\r\n")&&strstr(output,"503"));busy=0;
 sharedTarget="/";sharedResponse="HTTP/1.1 200 OK\r\n\r\nPanel";
 assert(serve("GET / HTTP/1.1\r\nHost: board\r\n\r\n")&&finished==1&&!strcmp(output,sharedResponse));
 sharedTarget="/lpc.cgi/A117?field=value";
 assert(serve("POST /lpc.cgi/A117 HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 11\r\n\r\nfield=value")&&finished==1);
 sharedTarget="/../secret";sharedResponse="HTTP/1.1 404 Not Found\r\n\r\n";
 assert(serve("GET /../secret HTTP/1.1\r\n\r\n")&&strstr(output,"404"));
 assert(serve("POST / HTTP/1.1\r\n\r\n")&&strstr(output,"400"));
 writeFailure=1;assert(!serve("GET /aaa.htm HTTP/1.1\r\n\r\n")&&closes==1&&!locked);
 puts("Ethernet HTTP: fragmented requests, short writes, 200k SD streaming, mutex release during I/O, failure cleanup and routes: PASS");
}
'''
with tempfile.TemporaryDirectory(prefix='network-http-') as d:
 p=Path(d)
 for name,content in headers.items():
  target=p/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(content)
 (p/'test.c').write_text(code)
 subprocess.run(['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(p),'-I'+str(root/'Application/NETWORK'),'-I'+str(root/'Application/HTTP'),str(p/'test.c'),str(root/'Application/HTTP/http_request_stream.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
