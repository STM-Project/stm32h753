"""Production font allocator: cache-line isolation and non-LIFO font deletion."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Application/LCD/LCD_fonts_images.c').read_text()
def fn(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
c=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#define MAX_OPEN_FONTS_SIMULTANEOUSLY 17
#define MAX_FONTS_AND_IMAGES_MEMORY_SIZE 2048
static char fontsImagesMemoryBuffer[2048] __attribute__((aligned(32)));
static struct {uint32_t fontSdramLenght;char *pointerToMemoryFont;} Font[17];
static struct {uint32_t offset,size;} fontAlloc[17];
static uint32_t CounterBusyBytesForFontsImages;
'''
c+=fn('static void ReleaseFontMemory(')+'\n'+fn('static bool DynamicFontMemoryAllocation(')
c+=r'''
int main(void){
 assert(!DynamicFontMemoryAllocation(0,0));assert(!DynamicFontMemoryAllocation(UINT32_MAX,0));
 assert(!DynamicFontMemoryAllocation(10,-1));assert(!DynamicFontMemoryAllocation(10,17));
 assert(DynamicFontMemoryAllocation(36,0));assert(DynamicFontMemoryAllocation(68,1));
 assert(fontAlloc[0].size==64&&fontAlloc[1].offset==64&&fontAlloc[1].size==96);
 /* Dirty tail bytes of glyph 0 must never share a line with the next font. */
 uintptr_t glyphTail=(uintptr_t)Font[0].pointerToMemoryFont+35;
 uintptr_t nextFont=(uintptr_t)Font[1].pointerToMemoryFont;
 assert((glyphTail&~(uintptr_t)31)<(nextFont&~(uintptr_t)31));
 memset(Font[1].pointerToMemoryFont,0x5a,68);char *live=Font[1].pointerToMemoryFont;
 ReleaseFontMemory(0);assert(CounterBusyBytesForFontsImages==160);
 assert(DynamicFontMemoryAllocation(65,2));assert(fontAlloc[2].offset==160);
 assert(!memcmp(live,"ZZZZ",4));
 assert(DynamicFontMemoryAllocation(32,0));assert(fontAlloc[0].offset==0);
 /* Serialized metadata cannot change the actual reservation or free boundary. */
 Font[1].fontSdramLenght=12;ReleaseFontMemory(1);
 assert(CounterBusyBytesForFontsImages==256);
 ReleaseFontMemory(2);assert(CounterBusyBytesForFontsImages==32);
 ReleaseFontMemory(0);assert(CounterBusyBytesForFontsImages==0);
 assert(DynamicFontMemoryAllocation(2048,0));assert(!DynamicFontMemoryAllocation(1,1));
 ReleaseFontMemory(0);
 for(int n=0;n<100;n++){
  for(int i=0;i<17;i++)assert(DynamicFontMemoryAllocation(4+4*(i%4),i));
  for(int i=0;i<17;i+=2)ReleaseFontMemory(i);
  for(int i=0;i<17;i+=2)assert(DynamicFontMemoryAllocation(4,i));
  for(int i=0;i<17;i++)ReleaseFontMemory(i);
  assert(CounterBusyBytesForFontsImages==0);
 }
 puts("PASS: 32-byte font blocks isolate glyph tails; non-LIFO delete/reuse, CFF-independent bookkeeping and allocation bounds.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(c)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

# CFF loading must reject truncated input, release failed slots and use memcpy
# for the header stored at the unaligned file offset 2.
loader = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
typedef uint32_t u32;typedef int FONTS_TYPES;
#define MAX_OPEN_FONTS_SIMULTANEOUSLY 17
#define MAX_FONTS_AND_IMAGES_MEMORY_SIZE 8192
#define MAX_CHARS 256
#define POSITION_AND_WIDTH 2
#define FR_OK 0
#define FA_READ 1
enum {RGB_RGB,Gray_Green,RGB_White,White_Black};
enum {DARKGRAY=1,MYGREEN=2,WHITE=3,BLACK=4};
static char fontsImagesMemoryBuffer[8192] __attribute__((aligned(32)));
static struct {uint32_t offset,size;} fontAlloc[17];
static uint32_t CounterBusyBytesForFontsImages;
'''
font_source=(root/'Application/LCD/LCD_fonts_images.c').read_text()
end=font_source.index('} FONTS_SETTING;')+len('} FONTS_SETTING;')
start=font_source.rfind('typedef struct{',0,end)
loader+=font_source[start:end]+'\nstatic FONTS_SETTING Font[17];\n'
loader+=r'''
static struct {uint32_t size,style,bkColor,color;} FontID[17];
#define SIZE_FONTID_TAB (2+sizeof(FONTS_SETTING)+sizeof(FontID[0]))
#define SIZE_AA_TAB 1502
static const char *BkColorFontFilePath[]={"","","",""},*ColorFontFilePath[]={"","","","",""};
static const char *StyleFontFilePath[]={""},*TxtFontSize[]={"font"},*TxtCFF=".cff";
static unsigned char file[8192];static uint32_t fileSize=8192,requested;
static int infoError,readError,closed;
static int SearchFontIndex(int a,int b,uint32_t c,uint32_t d){(void)a;(void)b;(void)c;(void)d;return -1;}
static int LCD_GetFontID(int a,int b,uint32_t c,uint32_t d){(void)a;(void)b;(void)c;(void)d;return 0;}
static int LoadFontIndex(int a,int b,uint32_t c,uint32_t d){(void)a;(void)b;(void)c;(void)d;Font[0].fontSizeToIndex=1;return 0;}
static int SDCardFileInfo(char *name,uint32_t *size){(void)name;*size=fileSize;return infoError;}
static int SDCardFileOpen(int id,char *name,int mode){(void)id;(void)name;(void)mode;return 0;}
static int SDCardFileRead(int id,char *out,uint32_t size){(void)id;requested=size;assert(size==fileSize);memcpy(out,file,size);return readError?(int)size-1:(int)size;}
static int SDCardFileClose(int id){(void)id;closed++;return 0;}
'''
loader+=fn('static void ReleaseFontMemory(')+'\n'+fn('static bool DynamicFontMemoryAllocation(')+'\n'+fn('int LCD_LoadFontFromFileCFF(')
loader+=r'''
int main(void){
 infoError=1;assert(LCD_LoadFontFromFileCFF(0,0,Gray_Green,0)==-3&&Font[0].fontSizeToIndex==0);
 infoError=0;fileSize=12;assert(LCD_LoadFontFromFileCFF(0,0,Gray_Green,0)==-9);
 fileSize=8192;readError=1;assert(LCD_LoadFontFromFileCFF(0,0,Gray_Green,0)==-5&&closed==1&&CounterBusyBytesForFontsImages==0);
 FONTS_SETTING header={.fontSizeToIndex=1,.fontSdramLenght=8192};
 memcpy(file+2,&header,sizeof header);readError=0;
 assert(LCD_LoadFontFromFileCFF(0,0,Gray_Green,0)==0);
 assert(requested==8192&&closed==2&&Font[0].pointerToMemoryFont==fontsImagesMemoryBuffer);
 assert(Font[0].fontSdramLenght==8192&&fontAlloc[0].size==8192);
 puts("PASS: CFF exact read size, short-file rejection, failure cleanup and unaligned header copy.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(loader)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-no-pie','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
