#include "http_settings_storage.h"
#include "http_settings_model.h"
#include "ff.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define N HTTP_SETTINGS_BLOB_SIZE
typedef struct { int exists; size_t size; unsigned char data[N]; } MockFile;
static MockFile files[3];
static unsigned char loaded[N], candidate[N];
static HTTP_SettingsSave saveCallback;
static void *saveContext;
static int locked, lockFail, failOpen, failRead, failWrite, failSync, failClose, failReadClose;
static int shortRead, shortWrite, failUnlink, failRotate, failPublish, failRollback;
static int openFiles, syncs, closes;
static int failReadback, corruptStaging, staleStaging;

static int pathId(const char *path) {
    if (!strcmp(path,"0:/h7conf.bin")) return 0;
    if (!strcmp(path,"0:/h7conf.bak")) return 1;
    assert(!strcmp(path,"0:/h7conf.tmp")); return 2;
}
unsigned TakeMutex(int id, unsigned ticks) {
    assert(id==2 && ticks==1000 && !locked);
    if (lockFail) return 0;
    locked=1; return 1;
}
void GiveMutex(int id) { assert(id==2 && locked); locked=0; }
FRESULT f_open(FIL *f, const char *path, unsigned flags) {
    assert(locked && !openFiles); if(failOpen) return FR_NOT_READY;
    int id=pathId(path);
    if(flags & FA_CREATE_ALWAYS) {files[id].exists=1;files[id].size=0;}
    else if(!files[id].exists) return FR_NO_FILE;
    f->id=id; f->size=files[id].size; openFiles++; return FR_OK;
}
FRESULT f_close(FIL *f) { assert(locked && openFiles==1);openFiles--;closes++;return ((failClose&&f->id==2)||(failReadClose&&f->id!=2))?FR_DISK_ERR:FR_OK; }
FRESULT f_read(FIL *f, void *dst, UINT len, UINT *count) {
    assert(locked && openFiles); if(failRead || (failReadback && f->id==2)) return FR_DISK_ERR;
    *count=shortRead?len-1:len;memcpy(dst,files[f->id].data,*count);return FR_OK;
}
FRESULT f_write(FIL *f,const void *src,UINT len,UINT *count) {
    assert(locked && openFiles);if(failWrite) return FR_DISK_ERR;
    *count=shortWrite?len-1:len;memcpy(files[f->id].data,src,*count);files[f->id].size=*count;
    if (corruptStaging) files[f->id].data[3] ^= 0xff;
    if (staleStaging) memset(files[f->id].data,7,*count);
    return FR_OK;
}
FRESULT f_sync(FIL *f) { (void)f;assert(locked && openFiles);syncs++;return failSync?FR_DISK_ERR:FR_OK; }
FRESULT f_unlink(const char *path) {
    assert(locked && !openFiles);int id=pathId(path);
    if(failUnlink) return FR_DISK_ERR;
    if(!files[id].exists)return FR_NO_FILE;
    files[id].exists=0;return FR_OK;
}
FRESULT f_rename(const char *old,const char *new) {
    assert(locked && !openFiles);int a=pathId(old),b=pathId(new);
    if((a==0&&b==1&&failRotate)||(a==2&&b==0&&failPublish)||(a==1&&b==0&&failRollback))return FR_DISK_ERR;
    if(!files[a].exists)return FR_NO_FILE;
    if(files[b].exists)return FR_EXIST;
    files[b]=files[a];files[a].exists=0;return FR_OK;
}
int HTTP_SettingsModelValidateBlob(const void *ptr,size_t len) {
    const unsigned char *p=ptr;if(!p||len!=N||!p[0])return 0;
    for(size_t i=1;i<len;i++)if(p[i]!=p[0])return 0;
    return 1;
}
int HTTP_SettingsModelInit(HTTP_SettingsLoad load,HTTP_SettingsSave save,void *ctx) {
    saveCallback=save;saveContext=ctx;return load(loaded,N,ctx);
}
static void reset(void) {
    memset(files,0,sizeof files);memset(loaded,0,N);
    lockFail=failOpen=failRead=failWrite=failSync=failClose=failReadClose=0;
    shortRead=shortWrite=failUnlink=failRotate=failPublish=failRollback=0;
    assert(!locked&&!openFiles);syncs=closes=0;
}
static void seed(int id,int value) { files[id].exists=1;files[id].size=N;memset(files[id].data,value,N); }
static void expectValue(int id,int value) { assert(files[id].exists&&files[id].size==N);for(size_t i=0;i<N;i++)assert(files[id].data[i]==value); }
static int save(int value) {memset(candidate,value,N);int r=saveCallback(candidate,N,saveContext);assert(!locked&&!openFiles);return r;}
int main(void) {
    reset();assert(HTTP_SettingsStorageInit()==0);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_DEFAULTS);
    assert(save(1)==1);expectValue(0,1);assert(!files[1].exists&&!files[2].exists);assert(syncs==1);
    assert(save(2)==1);expectValue(0,2);expectValue(1,1);assert(HTTP_SettingsStorageInit()==1&&loaded[0]==2);
    files[0].data[3]=9;assert(HTTP_SettingsStorageInit()==1&&loaded[0]==1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_RECOVERED_BACKUP);
    assert(save(3)==1);expectValue(0,3);expectValue(1,1);
    reset();seed(0,1);files[0].data[3]=9;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_CORRUPT);assert(save(2)==-1);assert(files[0].data[3]==9&&!files[2].exists);
    reset();seed(1,1);assert(HTTP_SettingsStorageInit()==1&&loaded[0]==1);assert(save(2)==1);expectValue(0,2);expectValue(1,1);
    reset();seed(0,1);files[0].size--;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_CORRUPT);
    reset();seed(0,1);shortRead=1;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_CORRUPT);
    reset();seed(0,1);failRead=1;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_IO_ERROR);assert(HTTP_SettingsStorageGetFatFsError()==FR_DISK_ERR);
    reset();seed(0,1);failReadClose=1;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_IO_ERROR);
    reset();seed(0,1);seed(1,2);files[0].data[3]=9;files[1].data[3]=9;assert(HTTP_SettingsStorageInit()==-1);assert(save(3)==-1);assert(files[0].data[3]==9&&files[1].data[3]==9);
    reset();seed(1,2);files[1].data[3]=9;assert(HTTP_SettingsStorageInit()==-1);assert(save(3)==-1);assert(!files[0].exists&&files[1].data[3]==9);
    reset();lockFail=1;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_LOCK_TIMEOUT);
    reset();failOpen=1;assert(HTTP_SettingsStorageInit()==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_IO_ERROR);assert(save(2)==-1);
    assert(strstr(HTTP_SettingsStorageGetErrorMessage(),"Kod FatFs:"));
    failOpen=0;
    assert(HTTP_SettingsStorageRetryLoad()==1);
    assert(save(2)==1);expectValue(0,2);
    assert(HTTP_SettingsStorageRetryLoad()==0); /* Never reload a live model. */
    for(int i=0;i<11;i++) {
        reset();seed(0,1);seed(1,4);assert(HTTP_SettingsStorageInit()==1);
        switch(i){case 0:failWrite=1;break;case 1:shortWrite=1;break;case 2:failSync=1;break;case 3:failClose=1;break;case 4:failUnlink=1;break;case 5:failRotate=1;break;case 6:failPublish=1;break;case 7:failPublish=failRollback=1;break;case 8:failReadback=1;break;case 9:corruptStaging=1;break;case 10:staleStaging=1;break;}
        assert(save(2)==-1);assert(HTTP_SettingsStorageGetStatus()==HTTP_SETTINGS_STORAGE_IO_ERROR);
        if(i==7){assert(!files[0].exists);expectValue(1,1);}else expectValue(0,1);
        failWrite=shortWrite=failSync=failClose=failUnlink=failRotate=failPublish=failRollback=failReadback=corruptStaging=staleStaging=0;
        assert(HTTP_SettingsStorageInit()==1&&loaded[0]==1);
    }
    reset();seed(0,1);files[0].data[3]=9;seed(1,4);assert(HTTP_SettingsStorageInit()==1&&loaded[0]==4);failPublish=1;assert(save(2)==-1);expectValue(1,4);assert(!files[0].exists);failPublish=0;assert(HTTP_SettingsStorageInit()==1&&loaded[0]==4);
    reset();seed(0,1);seed(1,4);files[1].data[3]=9;assert(HTTP_SettingsStorageInit()==1);assert(save(2)==1);expectValue(0,2);expectValue(1,1);
    reset();assert(HTTP_SettingsStorageInit()==0);memset(candidate,1,N);candidate[3]=2;assert(saveCallback(candidate,N,saveContext)==-1);assert(!files[0].exists);
    puts("PASS: fresh/load/save, blob and size corruption, backup recovery, mutex, short I/O, sync/close failures, staging readback corruption/stale write, rotation and publish rollback.");
}
