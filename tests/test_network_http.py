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
#define NETCONN_TCP 1
#define NETCONN_COPY 1
#define IP_ADDR_ANY 0
struct netconn *netconn_new(int);
int netconn_recv(struct netconn*,struct netbuf**);
unsigned short netbuf_len(struct netbuf*);
unsigned short netbuf_copy_partial(struct netbuf*,void*,unsigned short,unsigned short);
void netbuf_delete(struct netbuf*);
int netconn_write(struct netconn*,const void*,size_t,int);
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
int main(void){
 assert(serve("GET /test HTTP/1.1\r\nHost: board\r\n\r\n"));assert(strstr(output,"200 OK")&&strstr(output,"TLS-test")&&!closes);
 fileSize=200000;assert(serve("GET / HTTP/1.1\r\n\r\n")&&closes==1&&readCount>90);
 char *body=strstr(output,"\r\n\r\n")+4;assert(strlen(body)==fileSize);for(unsigned i=0;i<fileSize;i++)assert(body[i]=='a'+i%26);
 missing=1;assert(serve("GET / HTTP/1.1\r\n\r\n")&&strstr(output,"503 Service Unavailable")&&!closes);missing=0;
 busy=1;assert(serve("GET / HTTP/1.1\r\n\r\n")&&strstr(output,"503"));busy=0;
 assert(serve("GET /../secret HTTP/1.1\r\n\r\n")&&strstr(output,"404"));
 assert(serve("POST / HTTP/1.1\r\n\r\n")&&strstr(output,"405"));
 writeFailure=1;assert(!serve("GET / HTTP/1.1\r\n\r\n")&&closes==1&&!locked);
 puts("Ethernet HTTP: fragmented requests, short writes, 200k SD streaming, mutex release during I/O, failure cleanup and routes: PASS");
}
'''
with tempfile.TemporaryDirectory(prefix='network-http-') as d:
 p=Path(d)
 for name,content in headers.items():
  target=p/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(content)
 (p/'test.c').write_text(code)
 subprocess.run(['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(p),'-I'+str(root/'Application/NETWORK'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
