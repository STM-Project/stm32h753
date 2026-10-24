/* Frozen pre-optimization renderer for pixel-equivalence checks. */
static void ThresholdRoundReference(int x,int y,int w,int h,int radius,uint32_t top,uint32_t bottom)
{
    for(int j=0;j<h;j++)for(int i=0;i<w;i++){
        float dx=i<radius?radius-i-0.5f:i>=w-radius?i-(w-radius)+0.5f:0;
        float dy=j<radius?radius-j-0.5f:j>=h-radius?j-(h-radius)+0.5f:0;
        float coverage=dx>0&&dy>0?radius+0.5f-sqrtf(dx*dx+dy*dy):1;
        if(coverage<=0)continue;
        uint32_t c=Mix(top,bottom,h>1?(unsigned)(j*255/(h-1)):0);
        if(coverage<1)c=Mix(pLcd[(y+j)*800+x+i],c,(unsigned)(coverage*255));
        LCD_Buffer(800,x+i,y+j,c);
    }
}
