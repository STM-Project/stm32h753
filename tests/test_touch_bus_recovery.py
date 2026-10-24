"""Production bus-clear code against modeled open-drain SDA/SCL lines."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Application/LCD/Touch/stmpe811.c').read_text()
a=s.index('static int I2C1_WaitSclHigh(')
b=s.index('static void I2Cx_Error(',a)
c=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define GPIO_PIN_7 128
#define GPIO_PIN_8 256
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define GPIO_MODE_OUTPUT_OD 3
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_LOW 0
#define GPIOB 1
static int hi2c1, latch, pulses, clocksLow, sdaStuck, releaseAfter, inits, resets, deinit, delays;
typedef struct {int Pin,Mode,Pull,Speed;} GPIO_InitTypeDef;
void HAL_I2C_DeInit(int *h){assert(h==&hi2c1);deinit++;}
void HAL_GPIO_Init(int port,GPIO_InitTypeDef *p){assert(port==GPIOB&&p->Pin==(128|256)&&p->Mode==GPIO_MODE_OUTPUT_OD&&p->Pull==GPIO_NOPULL);}
void HAL_GPIO_WritePin(int port,int pin,int value){
 assert(port==GPIOB);
 if(pin==GPIO_PIN_8&&value&&!(latch&GPIO_PIN_8))pulses++;
 if(value)latch|=pin;else latch&=~pin;
}
int HAL_GPIO_ReadPin(int port,int pin){
 assert(port==GPIOB);
 if(pin==GPIO_PIN_8)return !clocksLow&&!!(latch&pin);
 assert(pin==GPIO_PIN_7);
 return !!(latch&pin)&&!sdaStuck&&pulses>=releaseAfter;
}
void HAL_Delay(unsigned ms){assert(ms==1);assert(++delays<100);}
void MX_I2C1_Init(void){assert(deinit==1);inits++;}
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C1_FORCE_RESET() (++resets)
#define __HAL_RCC_I2C1_RELEASE_RESET() (++resets)
''' + s[a:b] + r'''
static void reset(void){latch=384;pulses=clocksLow=sdaStuck=releaseAfter=inits=resets=deinit=delays=0;}
static void restored(void){assert(inits==1&&resets==2&&deinit==1&&latch==384);}
int main(void){
 reset();assert(BSP_TS_RecoverBus()==1&&pulses==1);restored();
 reset();releaseAfter=3;assert(BSP_TS_RecoverBus()==1&&pulses==4);restored();
 reset();sdaStuck=1;assert(BSP_TS_RecoverBus()==0&&pulses==10);restored();
 reset();clocksLow=1;assert(BSP_TS_RecoverBus()==0&&pulses==0&&delays<=6);restored();
 puts("Touch I2C recovery: idle, SDA released after clocks, stuck SDA/SCL, bounded waits and unconditional peripheral restore: PASS");
}
'''
with tempfile.TemporaryDirectory(prefix='touch-recovery-') as d:
 p=Path(d);(p/'test.c').write_text(c)
 subprocess.run(['gcc','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
