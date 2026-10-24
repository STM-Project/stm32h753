"""Exercise production SD startup retries and touch initialization validation."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(path, signature):
    source = (root/path).read_text(encoding='latin1')
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

code = r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef int FRESULT;
enum {FR_OK,FR_NOT_READY,FR_DISK_ERR,FR_NO_PATH};
int SDFatFS, delays, errors, mounts, failCount, failure;
char SDPath[]="0:";
typedef char TCHAR;
int f_mount(void *fs,const char *path,int now){
 assert(fs==&SDFatFS&&path==SDPath&&now==1);
 return mounts++<failCount?failure:FR_OK;
}
void osDelay(unsigned ms){assert(ms==250);delays++;}
void ERROR_SDcardMount(int result){assert(result==failure);errors++;}
''' + function('Application/General_Utilities/sd_card.c', 'void SDCard_Init(void)') + r'''
enum {TS_ERROR=1,TS_OK=0,TS_I2C_ADDRESS=0x82,STMPE811_ID=0x811,
 TS_SWAP_Y=2,STMPE811_REG_TSC_CTRL=0x40,STMPE811_REG_INT_CTRL=9,
 STMPE811_TS_CTRL_ENABLE=1,STMPE811_GIT_EN=1};
static uint8_t io_failed;
static int ts_x_boundary,ts_y_boundary,ts_orientation;
static int badID,badWrite,badCtrl,badIRQ,inits;
static unsigned readID(uint16_t addr){assert(addr==0x82);return badID?0:STMPE811_ID;}
static void init(uint16_t addr){assert(addr==0x82);inits++;if(badWrite)io_failed=1;}
static void noop(uint16_t addr){assert(addr==0x82);}
static uint8_t IOE_Read(uint16_t addr,int reg){assert(addr==0x82);return reg==0x40?!badCtrl:!badIRQ;}
typedef struct {unsigned (*ReadID)(uint16_t);void (*Init)(uint16_t),(*Start)(uint16_t),(*ClearIT)(uint16_t),(*EnableIT)(uint16_t);} Driver;
static Driver stmpe811_ts_drv={readID,init,noop,noop,noop},*ts_driver;
''' + function('Application/LCD/Touch/stmpe811.c', 'uint8_t BSP_TS_Init(') + r'''
int main(void){
 failure=FR_NOT_READY;failCount=2;SDCard_Init();assert(mounts==3&&delays==2&&!errors);
 mounts=delays=errors=0;failCount=5;SDCard_Init();assert(mounts==3&&delays==2&&errors==1);
 mounts=delays=errors=0;failure=FR_NO_PATH;SDCard_Init();assert(mounts==1&&!delays&&errors==1);
 assert(BSP_TS_Init(800,480)==TS_OK&&ts_driver&&ts_x_boundary==800&&ts_y_boundary==480);
 badID=1;assert(BSP_TS_Init(800,480)==TS_ERROR&&!ts_driver&&inits==1);badID=0;
 badWrite=1;assert(BSP_TS_Init(800,480)==TS_ERROR&&!ts_driver);badWrite=0;
 badCtrl=1;assert(BSP_TS_Init(800,480)==TS_ERROR&&!ts_driver);badCtrl=0;
 badIRQ=1;assert(BSP_TS_Init(800,480)==TS_ERROR&&!ts_driver);badIRQ=0;
 assert(BSP_TS_Init(800,480)==TS_OK&&ts_driver&&!io_failed);
 puts("Peripheral startup: bounded SD retries, permanent error handling, STMPE811 configuration validation/recovery: PASS");
}
'''
with tempfile.TemporaryDirectory(prefix='peripheral-startup-') as directory:
    work=Path(directory)
    (work/'test.c').write_text(code)
    subprocess.run(['gcc','-Wall','-Wextra','-Werror',str(work/'test.c'),'-o',str(work/'test')],check=True)
    subprocess.run([str(work/'test')],check=True)
