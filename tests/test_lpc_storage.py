#!/usr/bin/env python3
"""Exercise real SD page loading with short reads, errors and bounded DMA chunks."""
from pathlib import Path
import subprocess,tempfile
from test_http_sensor_storage import FATFS_STUB,TIMER_STUB
ROOT=Path(__file__).resolve().parents[1]
C=r'''
#include "lpc_pages.h"
#include "ff.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static int held,locked,missing,read_error,short_read,close_error;
static size_t length=636799,position,reads;
unsigned TakeMutex(int id,unsigned timeout){(void)id;(void)timeout;assert(!held);if(locked)return 0;held=1;return 1;}
void GiveMutex(int id){(void)id;assert(held);held=0;}
int LPC_ValidateSettings(const void *p,size_t n){(void)p;(void)n;return 1;}
int LPC_SettingsInit(LpcLoad a,LpcSave b,void*c){(void)a;(void)b;(void)c;return 0;}
FRESULT f_open(FIL*f,const char*path,unsigned flags){assert(held);assert(flags==FA_READ);assert(!strcmp(path,"0:/LPC/P00012.HTM"));if(missing)return FR_NO_FILE;f->size=length;position=0;reads=0;return FR_OK;}
FRESULT f_read(FIL*f,void*out,UINT size,UINT*got){(void)f;assert(held);assert(size<=4096);assert(!((size_t)out%32));reads++;if(read_error&&reads==2)return FR_DISK_ERR;*got=short_read?size-1:size;for(UINT i=0;i<*got;i++)((unsigned char*)out)[i]=(unsigned char)((position+i)%251+1);position+=*got;return FR_OK;}
FRESULT f_close(FIL*f){(void)f;assert(held);return close_error?FR_DISK_ERR:FR_OK;}
FRESULT f_write(FIL*f,const void*p,UINT n,UINT*r){(void)f;(void)p;(void)n;(void)r;return FR_DISK_ERR;}
FRESULT f_sync(FIL*f){(void)f;return FR_OK;}
FRESULT f_unlink(const char*p){(void)p;return FR_OK;}
FRESULT f_rename(const char*a,const char*b){(void)a;(void)b;return FR_OK;}
static char buffer[LPC_PAGE_BUFFER_SIZE+32];
int main(void){
 assert(LPC_StorageInit()==0);
 memset(buffer,0x55,sizeof buffer);
 assert(LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE)==length);
 for(size_t i=0;i<length;i++)assert((unsigned char)buffer[i]==i%251+1);
 assert(!buffer[length]);assert(buffer[LPC_PAGE_BUFFER_SIZE]==0x55);assert(reads>100);assert(!held);
 missing=1;assert(!LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE));assert(strstr(LPC_StoragePageError(),"P00012.HTM"));missing=0;
 read_error=1;assert(!LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE));assert(!buffer[0]);read_error=0;
 short_read=1;assert(!LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE));short_read=0;
 close_error=1;assert(!LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE));close_error=0;
 length=LPC_PAGE_BUFFER_SIZE;assert(!LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE));
 locked=1;assert(!LPC_StorageReadPage(12,buffer,LPC_PAGE_BUFFER_SIZE));assert(strstr(LPC_StoragePageError(),"mutex"));locked=0;
 assert(!held);assert(!LPC_StorageReadPage(0,buffer,LPC_PAGE_BUFFER_SIZE));
 puts("PASS: SD page 636799 bytes, aligned 4KiB DMA staging, exact content, missing/oversized files, lock/short-read/read/close failures.");
}
'''
with tempfile.TemporaryDirectory() as name:
 p=Path(name);(p/'ff.h').write_text(FATFS_STUB);(p/'timer.h').write_text(TIMER_STUB);(p/'test.c').write_text(C)
 exe=p/'test'
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(p),'-I'+str(ROOT/'Application/LPC2148'),str(p/'test.c'),str(ROOT/'Application/LPC2148/lpc_storage.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
