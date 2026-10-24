/* Frozen pre-optimization renderer for pixel-equivalence checks. */
static void GaugeLabelReference(int index,int word,int selected)
{
    int id=(index?3:0)+word;
    float theta=(word==0?136.0f:word==1?90.0f:44.0f)*0.01745329252f;
    float rotation=1.57079632679f-theta,cs=cosf(rotation),sn=sinf(rotation);
    float radius=(gaugeGeometry[index].r+gaugeGeometry[index].band)*0.5f;
    float cx=gaugeGeometry[index].x+radius*cosf(theta);
    float cy=gaugeGeometry[index].y-radius*sinf(theta);
    float hw=gauges.label[id].width*0.5f,hh=gauges.label[id].height*0.5f;
    int extent=(int)ceilf(hw+hh)+2;
    uint32_t color=word==selected?0xFF444442U:0xFFADADABU;
    /* Inverse mapping and bilinear coverage preserve the native SD font's AA
     * while rotating the text tangentially along the grey arc.
     */
    for(int y=(int)cy-extent;y<=(int)cy+extent;y++)
        for(int x=(int)cx-extent;x<=(int)cx+extent;x++){
            float dx=x+0.5f-cx,dy=y+0.5f-cy;
            float u=cs*dx+sn*dy+hw-0.5f,v=-sn*dx+cs*dy+hh-0.5f;
            int ix=(int)floorf(u),iy=(int)floorf(v);
            float fx=u-ix,fy=v-iy;
            float alpha=(1-fy)*((1-fx)*GaugeMask(id,ix,iy)+fx*GaugeMask(id,ix+1,iy))+
                         fy*((1-fx)*GaugeMask(id,ix,iy+1)+fx*GaugeMask(id,ix+1,iy+1));
            if(alpha>0)PanelPixel(x,y,color,(unsigned)(alpha+0.5f));
        }
}

static void GaugeNeedleReference(int index,int shadow)
{
    float angle=(160.0f-(float)lcdGaugePositions[index]*0.14f)*0.01745329252f;
    float cs=cosf(angle),sn=sinf(angle),length=gaugeGeometry[index].length,base=gaugeGeometry[index].base;
    float norm=sqrtf(1.0f+base*base/(length*length));
    int x,y,w,h;GaugeRect(index,&x,&y,&w,&h);
    for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++){
        float dx=xx+0.5f-gaugeGeometry[index].x-(shadow?3:0);
        float dy=yy+0.5f-gaugeGeometry[index].pivot-(shadow?2:0);
        float u=dx*cs-dy*sn,v=dx*sn+dy*cs;
        float coverage;
        if(u<0)coverage=base+0.5f-sqrtf(u*u+v*v);
        else coverage=0.5f+fminf(length-u,(base*(1-u/length)-fabsf(v))/norm);
        if(coverage<=0)continue;
        unsigned alpha=coverage>=1?255U:(unsigned)(coverage*255);
        float shade=(v/base+1.0f)*0.5f;
        if(shade<0)shade=0;
        if(shade>1)shade=1;
        uint32_t color=v<0?PanelMix(0xFF797975U,0xFF464743U,(unsigned)(shade*255)):
                                   PanelMix(0xFF222321U,0xFF000000U,(unsigned)(shade*255));
        PanelPixel(xx,yy,shadow?0xFF000000U:color,shadow?alpha*38/255:alpha);
    }
}

static void GaugeDrawReference(int index)
{
    int x,y,w,h;GaugeRect(index,&x,&y,&w,&h);
    for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++){
        float dx=xx+0.5f-gaugeGeometry[index].x,dy=yy+0.5f-gaugeGeometry[index].y;
        float radius=sqrtf(dx*dx+dy*dy);
        float angle=atan2f(-dy,dx)*57.2957795131f;
        uint32_t color=PANEL_WHITE;
        float angular=fminf(angle-20.0f,160.0f-angle)*0.01745329252f*radius;
        float outer=fminf(gaugeGeometry[index].r+0.5f-radius,angular+0.5f);
        if(outer>0){
            float grey=fminf(outer,radius-gaugeGeometry[index].band+0.5f);
            if(grey>0)color=PanelMix(PANEL_WHITE,0xFFE6E6E5U,grey>=1?255:(unsigned)(grey*255));
            float band=fminf(gaugeGeometry[index].band+0.5f-radius,radius-gaugeGeometry[index].inner+0.5f);
            float coverage=fminf(band,angular+0.5f);
            if(coverage>0){
                float edgeWarm=(angle-70.0f)*0.01745329252f*radius+0.5f;
                float edgeCold=(angle-110.0f)*0.01745329252f*radius+0.5f;
                if(edgeWarm<0)edgeWarm=0;
                if(edgeWarm>1)edgeWarm=1;
                if(edgeCold<0)edgeCold=0;
                if(edgeCold>1)edgeCold=1;
                uint32_t sector=PanelMix(0xFFFF0800U,0xFFFFB600U,(unsigned)(edgeWarm*255));
                sector=PanelMix(sector,0xFF00B8F1U,(unsigned)(edgeCold*255));
                float f=(radius-gaugeGeometry[index].inner)/(gaugeGeometry[index].band-gaugeGeometry[index].inner);
                if(f<0)f=0;
                if(f>1)f=1;
                /* Dark rim shading fades into bright colour toward the hole. */
                float f2=f*f,f4=f2*f2;
                unsigned shade=(unsigned)(85.0f*f4*f2*f);
                sector=PanelMix(sector,0xFF333323U,shade);
                color=PanelMix(color,sector,coverage>=1?255:(unsigned)(coverage*255));
            }
        }
        LCD_Buffer(800,xx,yy,color);
    }
    int selected=lcdGaugePositions[index]<357?0:lcdGaugePositions[index]>643?2:1;
    for(int word=0;word<3;word++)GaugeLabelReference(index,word,selected);
    GaugeNeedleReference(index,1);GaugeNeedleReference(index,0);
}
