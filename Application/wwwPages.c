/*
 * wwwPages.c
 *
 *  Created on: 23.12.2020
 *      Author: RafalMar
 */

#include "wwwPages.h"
#include "HTTP/http_settings_model.h"
#include "relay_states.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "sd_card.h"
#include "mini_printf.h"
#include "common.h"
#include "_debug.h"
#include "variables.h"
#include "FreeRTOS.h"
#include "ff.h"
#include "timer.h"

#define MAX_SIZE_ASP_NAME	100
#define MAX_SIZE_CGI_NAME	100
#define MAX_SIZE_CGI_VAL	100
#define CGI_GET_STRING	"GET /lpc.cgi/"

extern char* GETVAL_ptr();
extern char buff[];

/*
  var now=0;void setInterval(function(){now++;if(now==2){wst=0;}},50);\r\n\			//,50 -
  var myVar = setInterval(loadXMLDoc, 5000      );\r\n\								// 5000 - aby zawezic ten czas na np 500 to czas wyzej musi byc tez 50 a nie jak bylo zawsze 200
 */


//Test <div class="y" style="height:300px"><div class="x" style="height:270px"><font class="a"><b><div id="tu_temp001a"><font color="#eee">  21.6 °C</font></div></b></font><br><font class="b">Nazwa 1</font><div id="tu_lora001a"> </div></div></div>
//Test <div class="y" style="height:300px"><div class="x" style="height:270px"><font class="a"><b><div id="tu_temp001b"><font color="#eee">  21.6 °C</font></div></b></font><br><font class="b">Nazwa 1</font><div id="tu_lora001c"></div><br>                                <font class="a"><b><div id="tu_temp001c"><font color="#eee"> 93  </font></div></b></font><br><font class="b">Nazwa 2</font><div id="tu_lora001b"> </div></div></div>

char* HTTP_TEMPLATE_AdcTempRhu(char* buff, u8 type,u8 nr, float val1,float val2, u32 color1,u32 color2, char* name1,char* name2)	/* #f88 - #eee - #8cf */
{
	if(TEMPL_None!=type){
		/* char temp[30]={};		if(TEMPL_TempRhu==type) strcpy(temp," style=\"height:300px"); */
		if(TEMPL_TempRhu==type) buff += mini_snprintf(buff,200,"<div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\">\r\n");
		else					buff += mini_snprintf(buff,200,"<div class=\"y\">						<div class=\"x\">   				    \r\n");
	}

	if(TEMPL_Temp==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_temp%03da\"><font color=\"#%x\">%s °C</font></div></b></font><br><font class=\"b\">%s</font><div id=\"tu_lora%03da\"> </div>"
			,nr,color1, isnan(val1) ? " --.-" : Float2Str(val1,Space,3,Sign_minus,1), name1,nr );

	else if(TEMPL_TempRhu==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_temp%03db\"><font color=\"#%x\">%s °C   </font></div></b></font><br><font class=\"b\">%s</font><div id=\"tu_lora%03dc\"></div><br>\r\n\
				 						 <font class=\"a\"><b><div id=\"tu_temp%03dc\"><font color=\"#%x\">%s &#37;</font></div></b></font><br><font class=\"b\">%s</font><div id=\"tu_lora%03db\"> </div>\r\n"
			,nr,color1, isnan(val1) ? " --.-" : Float2Str(val1,Space,3,Sign_minus,1), name1,nr
			,nr,color2, isnan(val2) ? " --" : Float2Str(val2,Space,3,Sign_none, 0), name2,nr );

	else if(TEMPL_Ptt==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_ptt%03d\"><font color=\"#%x\">%s °C</font></div></b></font><br><font class=\"b\">%s</font>"
			,nr,color1, isnan(val1) ? " ---" : Float2Str(val1,Space,3,Sign_minus,1), name1 );

	else if(TEMPL_Adc==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_adc%03d\"><font color=\"#%x\">%s V</font></div></b></font><br><font class=\"b\">%s</font>"
			,nr,color1, isnan(val1) ? " --.-" : Float2Str(val1,Space,3,Sign_minus,1), name1 );

	if(TEMPL_None!=type)
		buff += mini_snprintf(buff,20,"</div></div>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_TestUsun(char* buff)	/* #f88 - #eee - #8cf */
{
	for(int i=1;i<=20;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_temp%03da\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora%03da\"> </div>",i,i);
	for(int i=3;i<=20;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_temp%03db\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora%03db\"> </div>",i,i);
	for(int i=3;i<=20;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_temp%03dc\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora%03dc\"> </div>",i,i);


	for(int i=21;i<=33;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_pk%03d\" style=\"display: none;\"><font color=\"#000\">  </font></div><div id=\"tu_lora%03dwy\"> </div>",i,i);
	for(int i=21;i<=33;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_ti%03d\" style=\"display: none;\"><font color=\"#000\">  </font></div>",i);
	for(int i=21;i<=33;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_te%03d\" style=\"display: none;\"><font color=\"#000\">  </font></div>",i);

	for(int i=1;i<=20;i++){ if(i!=15&&i!=16){
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_we%03d\" style=\"display: none;\"><font color=\"#eee\">  </font></div><div id=\"tu_lora%03dwe\"> </div>",i,i);
	}}
	for(int i=1;i<=14;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_eol%03d\" style=\"display: none;\"><font color=\"#f88\">  </font></div><div id=\"tu_lora%03deol\"> </div>",i,i);

	for(int i=1;i<=14;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_ptt%03d\" style=\"display: none;\"><font color=\"#eee\">  </font></div>",i);

	for(int i=18;i<=20;i++)//for(int i=17;i<=20;i++)
	{
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_imp0_%03d\" style=\"display: none;\">  <font color=\"#FFF\" size=\"6\">KWh  </font></div>",i);
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_imp1_%03d\" style=\"display: none;\"><font color=\"#eee\">        0.000 </font></div>",i);
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_imp2_%03d\" style=\"display: none;\"><font color=\"#eee\">        0.000 </font></div>",i);
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_imp3_%03d\" style=\"display: none;\"><font color=\"#eee\">        0.000 </font></div>",i);
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_imp4_%03d\" style=\"display: none;\"><font color=\"#eee\">        0.000 </font></div>",i);
	}

	for(int i=15;i<=16;i++)
		buff += mini_snprintf(buff,1000,"\r\n<div id=\"tu_adc%03d\" style=\"display: none;\"><font color=\"#eee\">  </font></div>",i);


	return buff;
}

char* HTTP_TEMPLATE_Impl(char* buff, u8 type,u8 nr, uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5, u32 color,char* name)
{
	(void)color;
	if(TEMPL_None!=type){
		buff += mini_snprintf(buff,200,"<div class=\"y\" style=\"height:330px\"><div class=\"x\" style=\"height:300px\">\r\n");
		buff += mini_snprintf(buff,1000,"<font class=\"z\" style=\"font-size: 43px;\"><b><div id=\"tu_imp0_%03d\">%d.%d<font color=\"#FFF\" size=\"6\"> KWh  </font></div></b></font><br>", 														   nr, val1/1000, val1-1000*(val1/1000) );
		buff += mini_snprintf(buff,1000,"<font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp1_%03d\"><font color=\"#eee\">%d.%d</font></div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;&nbsp;&nbsp;godzinny</font><br>", 				   nr, val2/1000, val2-1000*(val2/1000) );
		buff += mini_snprintf(buff,1000,"<font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp2_%03d\"><font color=\"#eee\">%d.%d</font></div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;dobowy </font><br>", nr, val3/1000, val3-1000*(val3/1000) );
		buff += mini_snprintf(buff,1000,"<font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp3_%03d\"><font color=\"#eee\">%d.%d</font></div></font><font color=\"#ccc\" size=\"4\"> &nbsp;tygodniowy</font><br>", 								   nr, val4/1000, val4-1000*(val4/1000) );
		buff += mini_snprintf(buff,1000,"<font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp4_%03d\"><font color=\"#eee\">%d.%d</font></div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;miesieczny</font><br>", 						   nr, val5/1000, val5-1000*(val5/1000) );
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font>",name );
		buff += mini_snprintf(buff,20,"</div></div>\r\n");
	}
	return buff;
}

char* HTTP_TEMPLATE_In(char* buff, u8 type, u8 nr, char* name1, u32 color, char* name2)
{
	if(TEMPL_None!=type){
		buff += mini_snprintf(buff,200,"<div class=\"y\"><div class=\"x\">\r\n");
	}

	if(TEMPL_In==type){
		buff += mini_snprintf(buff,1000,"<font class=\"z\"><b><div id=\"tu_we%03d\"><font color=\"#%x\">%s</font></div></b></font><br>",nr,color,name1);
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font><div id=\"tu_lora%03dwe\"> </div>",name2,nr);
	}

	else if(TEMPL_Eol==type){
		buff += mini_snprintf(buff,1000,"<font class=\"z\"><b><div id=\"tu_eol%03d\"><font color=\"#%x\">%s</font></div></b></font><br>",nr,color,name1);
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font><div id=\"tu_lora%03deol\"> </div>",name2,nr);
	}

	if(TEMPL_None!=type)
		buff += mini_snprintf(buff,20,"</div></div>\r\n");

	return buff;
}

char* HTTP_TEMPLATE_Out(char* buff, u8 type,u8 nr, float val, char* name1, char* name2)
{
	if(TEMPL_None!=type){
		buff += mini_snprintf(buff,200,"<div class=\"y\"><div class=\"x\">\r\n");
	}

	if(TEMPL_Pk==type){
		buff += mini_snprintf(buff,1000,"<a href=\"#\" rel=\"external\" onclick=\"xpk%03d(); this.href=&#39;javascript:return false;&#39;;\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk%03d\"><font color=\"#000\">%s</font></div></b></a><br>",nr,nr,name1);
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font><div id=\"tu_lora%03dwy\"> </div>",name2,nr);
	}

	else if(TEMPL_Ti==type){
		buff += mini_snprintf(buff,1000,"<a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti%03d\"><font color=\"#000\">%s</font></div></b></a><br><font class=\"b\">%s</font>",nr,name1,name2);
	}

	else if(TEMPL_Te==type){
		buff += mini_snprintf(buff,1000,"<a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te%03d\"><font color=\"#000\">%s</font></div></b></a><br><font class=\"b\">%s</font>",nr,name1,name2);
	}

	else if(TEMPL_Pwm==type){
		buff += mini_snprintf(buff,1000,"<table><tr><td width=\"100px\"><output  class=\"v\" style=\"font-size:36px;\" id=\"volume%03d\">%d</output></td><td></td><td><input type=\"range\" name=\"n_pwm%d\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"%d\"   oninput=\"outputUpdate%03d(value)\" onchange=\"x%03d(this.value);\"></input></td></tr></table><font class=\"b\">%s</font>",nr,(u8)val,nr-20,(u8)val,nr,nr,name1);
		//buff += mini_snprintf(buff,1000,"<table><tr><td width=\"100px\"><output  class=\"v\" style=\"font-size:36px;\" id=\"volume023\">45</output></td><td></td><td><input type=\"range\" name=\"n_pwm3\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"45\"   oninput=\"outputUpdate023(value)\" onchange=\"x023(this.value);\"></input></td></tr></table><font class=\"b\">AAAA</font>");
//		buff += mini_snprintf(buff,1000,"<table><tr><td width=\"100px\"><output  class=\"v\" style=\"font-size:36px;\" id=\"volume%03d\">%d</output></td><td></td><td><input type=\"range\" ",nr,(u8)val);
//		buff += mini_snprintf(buff,1000,"name=\"n_pwm%d\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"%d\"   ",nr-20,(u8)val);
//		buff += mini_snprintf(buff,1000,"oninput=\"outputUpdate%03d(value)\" onchange=\"x%03d(this.value);\"></input></td></tr></table><font class=\"b\">%s</font>",nr,nr,name1);
	}

	if(TEMPL_None!=type)
		buff += mini_snprintf(buff,20,"</div></div>\r\n");

	return buff;


//	<div class="y"><div class="x">
//	<a href="#" rel="external" onclick="xpk021(); this.href='javascript:return false;';" class="c" style="font-size: 25px;"><b><div id="tu_pk021"><font color="#000">Stan OFF X      </font></div></b></a>&nbsp;<span  id='tu_ant' style="display: inline-block; margin-top: 0px;"></span><br><font class="b">Wyjscie typu OC,IR,PWM</font>
//	</div></div>

//	<div class="y"><div class="x">
//	<a href="http://192.168.1.11/#" rel="external" onclick="xpk022(); this.href=&#39;javascript:return false;&#39;;" class="c" style="font-size: 25px;"><b><div id="tu_pk022"><font color="#000">Stan OFF             </font></div></b></a><br><font class="b">Wyjscie typu OC,IR,PWM</font><div id="tu_lora022wy"> </div>
//	</div></div>


//
//	<div class="y"><div class="x">
//	<a class="c" style="font-size: 25px;"><b><div id="tu_ti023"><font color="#000">Stan OFF A             </font></div></b></a><br><font class="b">Wyjscie typu OC,IR,PWM</font>
//	</div></div>
//
//	<div class="y"><div class="x">
//	<a class="c" style="font-size: 25px;"><b><div id="tu_te025"><font color="#000">Stan OFF             </font></div></b></a><br><font class="b">Wyjscie typu OC,IR</font>
//	</div></div>


}

/* Main read panel: saved /set modes select tiles, not demonstration calls. */
static u8 HTTP_MainPortTemplate(unsigned port)
{
	switch(HTTP_SettingsPortMode(port)){
	case HTTP_MODE_TEMP: return TEMPL_Temp;
	case HTTP_MODE_TEMP_RHU: return TEMPL_TempRhu;
	case HTTP_MODE_PT1000: return TEMPL_Ptt;
	case HTTP_MODE_ADC: return TEMPL_Adc;
	case HTTP_MODE_COUNTER: return TEMPL_Impl;
	case HTTP_MODE_INPUT: return TEMPL_In;
	case HTTP_MODE_EOL: return TEMPL_Eol;
	case HTTP_MODE_RELAY:
	case HTTP_MODE_IR: return TEMPL_Pk;
	case HTTP_MODE_TIMER: return TEMPL_Ti;
	case HTTP_MODE_THERMOSTAT: return TEMPL_Te;
	case HTTP_MODE_PWM: return TEMPL_Pwm;
	default: return TEMPL_None;
	}
}

static unsigned HTTP_MainTemplateGroup(u8 type)
{
	switch(type){
	case TEMPL_Temp:
	case TEMPL_TempRhu: return 0;
	case TEMPL_Impl: return 1;
	case TEMPL_In: return 2;
	case TEMPL_Eol: return 3;
	case TEMPL_Adc:
	case TEMPL_Ptt: return 4;
	case TEMPL_Pwm: return 5;
	case TEMPL_Pk: return 6;
	case TEMPL_Ti:
	case TEMPL_Te: return 7;
	default: return 8;
	}
}

static int HTTP_MainAppend(char* out, size_t capacity, size_t* used, const char* tile)
{
	size_t length=strlen(tile);
	if(length>=capacity-*used) return 0;
	memcpy(out+*used,tile,length+1);
	*used+=length;
	return 1;
}

size_t HTTP_MainReadPanelRender(char* out, size_t capacity)
{
	/* Existing templates write fixed, individually bounded fragments. This
	 * scratch buffer holds a complete tile before copying into the response;
	 * it also avoids using the WiFi task's small stack for the HTML. */
	static char tile[8192];
	char name[30*6+1];
	size_t used=0;
	u8 types[33];
	if(!out || !capacity) return 0;
	out[0]=0;
	if(!HTTP_MainAppend(out,capacity,&used,"<div class=\"h7-read-grid\">")) goto too_small;
	for(unsigned port=1;port<=33;port++) types[port-1]=HTTP_MainPortTemplate(port);
	/* Preserve Sandwitch's order: sensors, counters, inputs, EOL, ADC/Pt1000,
	 * PWM, relay/IR, then timers and thermostats. */
	for(unsigned group=0;group<8;group++) for(unsigned port=1;port<=33;port++){
		u8 type=types[port-1];
		if(HTTP_MainTemplateGroup(type)!=group) continue;
		const char* saved=HTTP_SettingsPortName(port,NULL);
		char* next=name;
		/* /set names are limited to 30 bytes; escape before legacy templates
		 * interpolate them into HTML text. */
		for(unsigned i=0;saved && saved[i] && i<30;i++){
			const char* escaped=NULL;
			switch(saved[i]){
			case '&': escaped="&amp;"; break;
			case '<': escaped="&lt;"; break;
			case '>': escaped="&gt;"; break;
			case '"': escaped="&quot;"; break;
			case '\'': escaped="&#39;"; break;
			default: *next++=saved[i]; break;
			}
			if(escaped){size_t length=strlen(escaped);memcpy(next,escaped,length);next+=length;}
		}
		*next=0;
		tile[0]=0;
		switch(type){
		case TEMPL_Temp:
		case TEMPL_TempRhu:
		case TEMPL_Ptt:
		case TEMPL_Adc:
			/* No measured H7 port value is available here yet. Polling supplies
			 * the existing data stream after the page is loaded. */
			HTTP_TEMPLATE_AdcTempRhu(tile,type,port,NAN,NAN,0xeee,0xeee,name,name);
			break;
		case TEMPL_Impl:
			HTTP_TEMPLATE_Impl(tile,type,port,0,0,0,0,0,0xeee,name);
			break;
		case TEMPL_In:
		case TEMPL_Eol:
			HTTP_TEMPLATE_In(tile,type,port,"---",0xeee,name);
			break;
		case TEMPL_Pwm:
			{
				const RelayState* state=RELAY_GetState(port);
				HTTP_TEMPLATE_Out(tile,type,port,state && state->pwm_valid?state->pwm_percent:0,name,name);
			}
			break;
		default:
			{
				int state=RELAY_GetDisplayState(port);
				HTTP_TEMPLATE_Out(tile,type,port,0,state<0?"---":state?"Stan ON":"Stan OFF",name);
			}
			break;
		}
		if(!HTTP_MainAppend(out,capacity,&used,tile)) goto too_small;
	}
	if(!HTTP_MainAppend(out,capacity,&used,"</div>")) goto too_small;
	/* Polling addresses every supported mode, including inactive ones. Emit
	 * each ID once, keeping disabled ports invisible and active IDs unique. */
	for(unsigned port=1;port<=33;port++){
		u8 type=types[port-1];
		char* next=HTTP_TEMPLATE_Hidden2(tile,type,port);
		if(port>=15 && port<=16 && type!=TEMPL_Adc)
			next+=mini_snprintf(next,100,"<div id=\"tu_adc%03u\" style=\"display:none;\"></div>",port);
		if(port>=17 && port<=20 && type!=TEMPL_Impl)
			for(unsigned period=0;period<5;period++)
				next+=mini_snprintf(next,100,"<div id=\"tu_imp%u_%03u\" style=\"display:none;\"></div>",period,port);
		if(!HTTP_MainAppend(out,capacity,&used,tile)) goto too_small;
	}
	return used;
too_small:
	out[0]=0;
	return 0;
}

/* Separate tile templates for HttpMainReadPanel2. */
char* HTTP_TEMPLATE_AdcTempRhu2(char* buff, u8 type,u8 nr, float val1,float val2, u32 color1,u32 color2, char* name1,char* name2)	/* #f88 - #eee - #8cf */
{
	if(TEMPL_None!=type){
		if(TEMPL_TempRhu==type) buff += mini_snprintf(buff,200,"<div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\">\r\n");
		else					buff += mini_snprintf(buff,200,"<div class=\"y\">						<div class=\"x\">   				    \r\n");
	}

	if(TEMPL_Temp==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_temp%03da\"><font color=\"#%x\">%s °C</font></div></b></font><br><font class=\"b\">%s</font>"
		,nr,color1, Float2Str(val1,Space,3,Sign_minus,1), name1 );

	else if(TEMPL_TempRhu==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_temp%03db\"><font color=\"#%x\">%s °C   </font></div></b></font><br><font class=\"b\">%s</font><div id=\"tu_lora%03dc\"></div><br>\r\n\
			<font class=\"a\"><b><div id=\"tu_temp%03dc\"><font color=\"#%x\">%s &#37;</font></div></b></font><br><font class=\"b\">%s</font><div id=\"tu_lora%03db\"> </div>\r\n"
		,nr,color1, Float2Str(val1,Space,3,Sign_minus,1), name1,nr
							  ,nr,color2, Float2Str(val2,Space,3,Sign_none, 0), name2,nr );

	else if(TEMPL_Ptt==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_ptt%03d\"><font color=\"#%x\">%s °C</font></div></b></font><br><font class=\"b\">%s</font>"
		,nr,color1, Float2Str(val1,Space,3,Sign_minus,1), name1 );

	else if(TEMPL_Adc==type)
		buff += mini_snprintf(buff,1000,"<font class=\"a\"><b><div id=\"tu_adc%03d\"><font color=\"#%x\">%s V</font></div></b></font><br><font class=\"b\">%s</font>"
		,nr,color1, Float2Str(val1,Space,3,Sign_minus,1), name1 );

	if(TEMPL_None!=type)
		buff += mini_snprintf(buff,20,"</div></div>\r\n");
	if(TEMPL_Temp==type)
		buff += mini_snprintf(buff,100,"<div id=\"tu_lora%03da\"></div>\r\n",nr);
	return buff;
}

char* HTTP_TEMPLATE_In2(char* buff, u8 type, u8 nr, char* name1, u32 color, char* name2)
{
	if(TEMPL_None!=type){
		buff += mini_snprintf(buff,200,"<div class=\"y\"><div class=\"x\">\r\n");
	}

	if(TEMPL_In==type){
		buff += mini_snprintf(buff,1000,"<font class=\"z\"><b><div id=\"tu_we%03d\"><font color=\"#%x\">%s</font></div></b></font><br>",nr,color,name1);
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font>",name2);
	}

	else if(TEMPL_Eol==type){
		buff += mini_snprintf(buff,1000,"<font class=\"z\"><b><div id=\"tu_eol%03d\"><font color=\"#%x\">%s</font></div></b></font><br>",nr,color,name1);
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font>",name2);
	}

	if(TEMPL_None!=type)
		buff += mini_snprintf(buff,20,"</div></div>\r\n");

	if(TEMPL_In==type)
		buff += mini_snprintf(buff,100,"<div id=\"tu_lora%03dwe\"></div>\r\n",nr);
	else if(TEMPL_Eol==type)
		buff += mini_snprintf(buff,100,"<div id=\"tu_lora%03deol\"></div>\r\n",nr);
	return buff;
}

char* HTTP_TEMPLATE_Out2(char* buff, u8 type,u8 nr, float val, char* name1, char* name2)
{
	if(TEMPL_None!=type){
		buff += mini_snprintf(buff,200,"<div class=\"y\"><div class=\"x\">\r\n");
	}

	if(TEMPL_Pk==type){
		buff += mini_snprintf(buff,1000,"<a href=\"#\" rel=\"external\" onclick=\"xpk%03d(); this.href=&#39;javascript:return false;&#39;;\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk%03d\"><font color=\"#000\">%s</font></div></b></a><br>",nr,nr,name1);
		buff += mini_snprintf(buff,1000,"<font class=\"b\">%s</font>",name2);
	}

	else if(TEMPL_Ti==type){
		buff += mini_snprintf(buff,1000,"<a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti%03d\"><font color=\"#000\">%s</font></div></b></a><br><font class=\"b\">%s</font>",nr,name1,name2);
	}

	else if(TEMPL_Te==type){
		buff += mini_snprintf(buff,1000,"<a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te%03d\"><font color=\"#000\">%s</font></div></b></a><br><font class=\"b\">%s</font>",nr,name1,name2);
	}

	else if(TEMPL_Pwm==type){
		buff += mini_snprintf(buff,1000,"<table><tr><td width=\"100px\"><output  class=\"v\" style=\"font-size:36px;\" id=\"volume%03d\">%d</output></td><td></td><td><input type=\"range\" name=\"n_pwm%d\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"%d\"   oninput=\"outputUpdate%03d(value)\" onchange=\"x%03d(this.value);\"></input></td></tr></table><font class=\"b\">%s</font>",nr,(u8)val,nr-20,(u8)val,nr,nr,name1);
	}

	if(TEMPL_None!=type)
		buff += mini_snprintf(buff,20,"</div></div>\r\n");

	if(TEMPL_Pk==type)
		buff += mini_snprintf(buff,100,"<div id=\"tu_lora%03dwy\"></div>\r\n",nr);
	return buff;
}

/* Call once per port, after its visible template, to supply the inactive IDs.
 * type is the visible template type; TEMPL_None hides every field for the port.
 * Panel 2 uses input ports 5..10 and output ports 21..24, 28, 33.
 */
char* HTTP_TEMPLATE_Hidden2(char* buff, u8 type, u8 nr)
{
	if(nr<=20){
		if(TEMPL_Temp!=type)
			buff += mini_snprintf(buff,200,"<div id=\"tu_temp%03da\" style=\"display: none;\"></div><div id=\"tu_lora%03da\"></div>\r\n",nr,nr);
		if(TEMPL_TempRhu!=type){
			buff += mini_snprintf(buff,200,"<div id=\"tu_temp%03db\" style=\"display: none;\"></div><div id=\"tu_lora%03dc\"></div>\r\n",nr,nr);
			buff += mini_snprintf(buff,200,"<div id=\"tu_temp%03dc\" style=\"display: none;\"></div><div id=\"tu_lora%03db\"></div>\r\n",nr,nr);
		}
		if(TEMPL_In!=type)
			buff += mini_snprintf(buff,200,"<div id=\"tu_we%03d\" style=\"display: none;\"></div><div id=\"tu_lora%03dwe\"></div>\r\n",nr,nr);
		if(TEMPL_Eol!=type)
			buff += mini_snprintf(buff,200,"<div id=\"tu_eol%03d\" style=\"display: none;\"></div><div id=\"tu_lora%03deol\"></div>\r\n",nr,nr);
		if(TEMPL_Ptt!=type)
			buff += mini_snprintf(buff,100,"<div id=\"tu_ptt%03d\" style=\"display: none;\"></div>\r\n",nr);
	}else{
		if(TEMPL_Pk!=type)
			buff += mini_snprintf(buff,200,"<div id=\"tu_pk%03d\" style=\"display: none;\"></div><div id=\"tu_lora%03dwy\"></div>\r\n",nr,nr);
		if(TEMPL_Ti!=type)
			buff += mini_snprintf(buff,100,"<div id=\"tu_ti%03d\" style=\"display: none;\"></div>\r\n",nr);
		if(TEMPL_Te!=type)
			buff += mini_snprintf(buff,100,"<div id=\"tu_te%03d\" style=\"display: none;\"></div>\r\n",nr);
	}
	return buff;
}


/* Mode values are CGI codes, not HTTP_TEMPALTE_TYPE values. */
static char* HTTP_TEMPLATE_ModeSelect2(char* buff, u8 nr, u8 mode, const u8* modes, unsigned int count)
{
	static const char* const labels[14]={
		"Przekaznik", "Sterowanie PWM", "Temperatura", "Czujnik Pt1000",
		"Czujniki zwar-rozwar", "Pomiar Napiecia 0-10V", " -- -- -- ",
		"Licznik impulsow", "Sterowanie IR", "Temperatura Wilgotnosc",
		"Timer", "Termostat", "", "Wejscie EOL"
	};
	unsigned int i;
	int supported=0;
	for(i=0;i<count;i++) if(modes[i]==mode) supported=1;
	if(!supported) mode=HTTP_MODE_DISABLED;
	buff += mini_snprintf(buff,300,"<table><tr><td><form name=\"wybor%03d\" method=\"post\" action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n",nr);
	buff += mini_snprintf(buff,200,"<select name=\"n_GPIO_val_%03d\" data-http-mode=\"%03d\" style=\"width: 290px\">\r\n",nr,nr);
	for(i=0;i<count;i++)
		buff += mini_snprintf(buff,200,"<option value=\"%02dp%03d\"%s>%d &nbsp;%s</option>\r\n",modes[i],nr,modes[i]==mode ? " selected" : "",nr,labels[modes[i]]);
	buff += mini_snprintf(buff,100,"</select></form></td></tr></table>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_ModeInput2(char* buff, u8 nr, u8 mode)
{
	static const u8 sensor[]={HTTP_MODE_TEMP,HTTP_MODE_PT1000,HTTP_MODE_TEMP_RHU,HTTP_MODE_INPUT,HTTP_MODE_EOL,HTTP_MODE_DISABLED};
	static const u8 adc[]={HTTP_MODE_ADC,HTTP_MODE_DISABLED};
	static const u8 counter[]={HTTP_MODE_TEMP,HTTP_MODE_TEMP_RHU,HTTP_MODE_INPUT,HTTP_MODE_COUNTER,HTTP_MODE_DISABLED};
	if(nr<1 || nr>20) return buff;
	if(nr<=14) return HTTP_TEMPLATE_ModeSelect2(buff,nr,mode,sensor,sizeof(sensor)/sizeof(sensor[0]));
	if(nr<=16) return HTTP_TEMPLATE_ModeSelect2(buff,nr,mode,adc,sizeof(adc)/sizeof(adc[0]));
	return HTTP_TEMPLATE_ModeSelect2(buff,nr,mode,counter,sizeof(counter)/sizeof(counter[0]));
}

char* HTTP_TEMPLATE_ModeOutput2(char* buff, u8 nr, u8 mode)
{
	static const u8 pwm[]={HTTP_MODE_RELAY,HTTP_MODE_TIMER,HTTP_MODE_THERMOSTAT,HTTP_MODE_PWM,HTTP_MODE_IR,HTTP_MODE_DISABLED};
	static const u8 ir[]={HTTP_MODE_RELAY,HTTP_MODE_TIMER,HTTP_MODE_THERMOSTAT,HTTP_MODE_IR,HTTP_MODE_DISABLED};
	static const u8 relay[]={HTTP_MODE_RELAY,HTTP_MODE_TIMER,HTTP_MODE_THERMOSTAT,HTTP_MODE_DISABLED};
	if(nr<21 || nr>33) return buff;
	if(nr<=23) return HTTP_TEMPLATE_ModeSelect2(buff,nr,mode,pwm,sizeof(pwm)/sizeof(pwm[0]));
	if(nr<=27) return HTTP_TEMPLATE_ModeSelect2(buff,nr,mode,ir,sizeof(ir)/sizeof(ir[0]));
	return HTTP_TEMPLATE_ModeSelect2(buff,nr,mode,relay,sizeof(relay)/sizeof(relay[0]));
}

/* Append once after all mode forms. Missing mode sections are allowed. */
char* HTTP_TEMPLATE_ModeScript2(char* buff)
{
	buff += mini_snprintf(buff,100,"<script>\r\n");
	buff += mini_snprintf(buff,700,
		"function HTTPModeApply2(select){"
		"var nr=select.getAttribute('data-http-mode');"
		"for(var mode=0;mode<=13;mode++){"
		"var id=('0'+mode).slice(-2)+'p'+nr;"
		"var block=document.getElementById(id);"
		"if(block) block.style.display=id===select.value?'block':'none';}"
		"window['active_id'+nr]=select.value;}\r\n");
	buff += mini_snprintf(buff,600,
		"window.addEventListener('load',function(){"
		"var selects=document.querySelectorAll('select[data-http-mode]');"
		"for(var i=0;i<selects.length;i++){"
		"HTTPModeApply2(selects[i]);"
		"selects[i].addEventListener('change',function(){HTTPModeApply2(this);});}});\r\n");
	buff += mini_snprintf(buff,1000,
		"function HTTPModeSave2(){var f=document.createElement('form');f.method='post';f.action='/lpc.cgi/A117';function add(n,v){var i=document.createElement('input');i.type='hidden';i.name=n;i.value=v;f.appendChild(i);}add('h7_page','/set');add('h7_form','1');var a=document.querySelectorAll('select[data-http-mode]');for(var j=0;j<a.length;j++)add(a[j].name,a[j].value.slice(0,2));add('n_109_zapisz','');document.body.appendChild(f);f.submit();}\r\n");
	buff += mini_snprintf(buff,50,"</script>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_ModeSave2(char* buff)
{
	buff += mini_snprintf(buff,200,"<button type=\"button\" class=\"m\" onclick=\"HTTPModeSave2()\">Zapisz ustawienia trybu</button>\r\n");
	return buff;
}

SDRAM_ALIGN32 char HttpRefr_sdram[100000];

/* W NotePad nalezy zamienic '\r\n' -> '\\r\\n\\\r\n' */
char HttpRefr[]="22:59:08 &nbsp;26/07/25120000000000000 111111111111111111 55555555555555  25.10 65%0 -78.30 34%2  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0       6.0       5.7      ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0 V     V              0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0";

const char HttpStyle[]="<!DOCTYPE html>\r\n\
<html>\r\n\
<head>\r\n\
  <meta charset=\"UTF-8\">\r\n\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n\
   <link rel=\"icon\" href=\"data:;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABGdBTUEAALGPC/xhBQAAACRJREFUOE9jZGj4z0AaAGogCZGmGuQckowf1UBkcI0GKxEBBQA55GigBl4jdAAAAABJRU5ErkJggg==\">\r\n\
   <style>.c{border-width: 0px;text-decoration: none;color: #222222;font-size: 19px;background-color: #468ccf;border-radius: 10px;padding: 5px 13px 5px 13px;display:inline-block;}</style>\r\n\
  <style>.v{border:0px solid #c4c4c4;height:60px;width:74px;font-size:25px;color:#ffffff;background-color:#505050;}</style>\r\n\
  <style>.m{border-width: 0px;text-decoration: none;color: #202020;font-size: 20px;background-color: #e0e0e0;border-radius: 10px;padding: 5px 13px 5px 13px;}</style>\r\n\
  <style>.n{border-width: 0px;text-decoration: none;color: #202020;font-size: 18px;background-color: #e0e0e0;border-radius: 10px;padding: 5px 13px 5px 13px;}</style>\r\n\
  <style>.x{border-radius: 30px;float:left;width: 320px;height: 120px;padding: 1px;border: solid 4px #808080;background-color: #505050;text-align: center;overflow: auto;font-size: 40px;}</style>\r\n\
 <style>.y{border-radius: 0px;float:left;width: 350px;height: 150px;text-align: center;overflow: auto;}</style>\r\n\
 <style>.b{font-size: 21px;color: #9edf0f;display:inline-block;}</style>\r\n\
 <style>.a{font-size: 50px;color: #efefef;display:inline-block;}</style>\r\n\
 <style>.z{font-size: 30px;color: #efefef;display:inline-block;}</style>\r\n\
<style>\r\n\
  .textbox {\r\n\
    border: 1px solid #c4c4c4;\r\n\
    height: 20px;\r\n\
    width: 270px;\r\n\
    font-size: 17px;\r\n\
    padding: 4px 4px 4px 4px;\r\n\
	background-color: #BBBBBB;\r\n\
    background: -moz-linear-gradient(top, #FFF, #BBBBBB);\r\n\
    background: -webkit-gradient(linear, left top, left bottom, color-stop(0.0, #FFF), color-stop(1.0, #BBBBBB));\r\n\
\r\n\
    border-radius: 4px;\r\n\
    -moz-border-radius: 4px;\r\n\
    -webkit-border-radius: 4px;\r\n\
    box-shadow: 0px 0px 2px #d9d9d9;\r\n\
    -moz-box-shadow: 0px 0px 2px #d9d9d9;\r\n\
    -webkit-box-shadow: 0px 0px 2px #d9d9d9;\r\n\
	//font-family: Comic Sans MS;\r\n\
    font-size: 18px;\r\n\
}\r\n\
\r\n\
.textbox:focus {\r\n\
    outline: none;\r\n\
    border: 3px solid #7bc1f7;\r\n\
    box-shadow: 0px 0px 12px #7bc1f7;\r\n\
    -moz-box-shadow: 0px 0px 12px #7bc1f7;\r\n\
    -webkit-box-shadow: 0px 0px 12px #7bc1f7;\r\n\
}\r\n\
 </style>\r\n\
<style>\r\n\
  .textboxB {\r\n\
    border: 1px solid #c4c4c4;\r\n\
    color: #fff;\r\n\
    height: 20px;\r\n\
    font-size: 17px;\r\n\
    padding: 4px 4px 4px 4px;\r\n\
    border-radius: 4px;\r\n\
    font-size: 18px;\r\n\
}\r\n\
 </style>\r\n\
\r\n\
 <style>\r\n\
\r\n\
.shiny-button {\r\n\
  display: inline-block;\r\n\
  text-align: center;\r\n\
  border-width: 1px;\r\n\
  border-style: solid;\r\n\
  text-transform: uppercase;\r\n\
  text-decoration: none;\r\n\
  line-height: 1.1;\r\n\
  font-weight: normal;\r\n\
  font-family: sans-serif;\r\n\
  color: #FFFFFF;\r\n\
  font-size: 20px;\r\n\
  background-color: #73B10E;\r\n\
  border-color: #73B10E;\r\n\
  -webkit-border-radius: 2px;\r\n\
  -moz-border-radius: 2px;\r\n\
  border-radius: 6px;\r\n\
  padding: 3px 7px 3px 7px;\r\n\
}\r\n\
</style>\r\n\
\r\n\
\r\n\
<style>\r\n\
.css_button {\r\n\
    font-size: 23px;\r\n\
    font-family:Arial;\r\n\
    font-weight:normal;\r\n\
    -moz-border-radius: 26px;\r\n\
    -webkit-border-radius: 26px;\r\n\
    border-radius: 16px;\r\n\
    border: 1px solid #3866A3;\r\n\
    padding: 9px 25px;\r\n\
    text-shadow:1px 1px 0px #5E5E5E;\r\n\
    -moz-box-shadow:inset 1px 1px 0px 0px #BEE2F9;\r\n\
    -webkit-box-shadow:inset 1px 1px 0px 0px #BEE2F9;\r\n\
    box-shadow:inset 1px 1px 0px 0px #BEE2F9;\r\n\
    font-weight:normal;\r\n\
    text-decoration:none;\r\n\
    cursor: pointer;\r\n\
    background:-webkit-linear-gradient(90deg, #468ccf, #63b8ee);\r\n\
    background:-moz-linear-gradient(90deg, #468ccf, #63b8ee);\r\n\
    background:-ms-linear-gradient(90deg, #468ccf, #63b8ee);\r\n\
    background:linear-gradient(180deg, #63b8ee, #468ccf);\r\n\
    filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#63b8ee', endColorstr='#468ccf');\r\n\
    color:#FFFFFF;\r\n\
    display:inline-block;\r\n\
}\r\n\
</style>\r\n\
\r\n\
\r\n\
<style>\r\n\
.css-font-style{\r\n\
   font-size: 20px;\r\n\
   padding: 10px;\r\n\
   color: #ff0000;\r\n\
   border: solid 0px #000;\r\n\
   font-family: Arial, \"Helvetica Neue\", Helvetica, sans-serif;\r\n\
}\r\n\
</style>\r\n\
\r\n\
<style>\r\n\
.css-font-style2{\r\n\
   font-size: 20px;\r\n\
   padding: 10px;\r\n\
   color: #9edf0f;\r\n\
   border: solid 0px #000;\r\n\
   font-family: Arial, \"Helvetica Neue\", Helvetica, sans-serif;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
.css_list{\r\n\
   font-size: 20px;\r\n\
   padding: 4px;\r\n\
   color: #9edf0f;\r\n\
   border: solid 0px #000;\r\n\
   font-family: Arial, \"Helvetica Neue\", Helvetica, sans-serif;\r\n\
}\r\n\
</style>\r\n\
\r\n\
\r\n\
\r\n\
\r\n\
<style>\r\n\
.corner-radius{\r\n\
    border-radius: 30px;\r\n\
    float:left;\r\n\
    width: 290px;\r\n\
    height: 450px;\r\n\
    padding: 1px;\r\n\
    border: solid 4px #808080;\r\n\
    background-color: #505050;\r\n\
    text-align: center;\r\n\
    overflow: auto;\r\n\
    font-family: Arial,Helvetica,sans-serif;\r\n\
    font-size: 40px;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
.corner-radius3{\r\n\
    border-radius: 30px;\r\n\
    float:left;\r\n\
    width: 290px;\r\n\
    height: 540px;\r\n\
    padding: 1px;\r\n\
    border: solid 4px #808080;\r\n\
    background-color: #505050;\r\n\
    text-align: center;\r\n\
    overflow: auto;\r\n\
    font-family: Arial,Helvetica,sans-serif;\r\n\
    font-size: 40px;\r\n\
}\r\n\
</style>\r\n\
\r\n\
\r\n\
\r\n\
<style>\r\n\
.txt_shadow{\r\n\
   text-shadow: 1px 1px 1px #BFBFBF;\r\n\
   font-size: 37px;\r\n\
   color: #ED7300;\r\n\
   font-family: Arial Black, Gadget, sans-serif;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
.corner-radius2{\r\n\
    border-radius: 0px;\r\n\
    float:left;\r\n\
    width: 320px;\r\n\
    height: 480px;\r\n\
    text-align: center;\r\n\
    overflow: auto;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
.corner-radius4{\r\n\
    border-radius: 0px;\r\n\
    float:left;\r\n\
    width: 320px;\r\n\
    height: 570px;\r\n\
    text-align: center;\r\n\
    overflow: auto;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
#holder {\r\n\
    width: 100%;\r\n\
}\r\n\
\r\n\
#holder > div {\r\n\
    clear: both;\r\n\
    padding: 2%;\r\n\
    margin-bottom: 20px;\r\n\
    border-bottom: 1px solid #eee;\r\n\
    float: left;\r\n\
    width: 96%;\r\n\
}\r\n\
\r\n\
label {\r\n\
    display: inline;\r\n\
}\r\n\
\r\n\
.regular-checkbox {\r\n\
    display: none;\r\n\
}\r\n\
\r\n\
.regular-checkbox + label {\r\n\
    background-color: #fafafa;\r\n\
    border: 1px solid #cacece;\r\n\
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05), inset 0px -15px 10px -12px rgba(0, 0, 0, 0.05);\r\n\
    padding: 9px;\r\n\
    border-radius: 3px;\r\n\
    display: inline-block;\r\n\
    position: relative;\r\n\
}\r\n\
.regular-checkbox:checked + label:after {\r\n\
 content: \"\";\r\n\
 position: absolute;\r\n\
 top: 4px;\r\n\
 left: 4px;\r\n\
 height: 50%;\r\n\
 width: 50%;\r\n\
 background-color: #090;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
­iv { margin: 20px; }\r\n\
#mainselection select, select[data-http-mode] {\r\n\
   border: 0;\r\n\
   color: #EEE;\r\n\
   background: #505050;\r\n\
   font-size: 20px;\r\n\
   padding: 2px 5px;\r\n\
   /*background: #58B14C;\r\n\
}\r\n\
</style>\r\n\
<script>\r\n\
	function enableChart()\r\n\
	{\r\n\
		if( document.getElementById('n_s_Lan_param_001').value == \"nie\")document.getElementById('n_s_Lan_ip_001').disabled=false;\r\n\
		else document.getElementById('n_s_Lan_ip_001').disabled = true;\r\n\
\r\n\
		if( document.getElementById('n_s_Lan_param_001').value == \"nie\")document.getElementById('n_s_Lan_mask_001').disabled=false;\r\n\
		else document.getElementById('n_s_Lan_mask_001').disabled = true;\r\n\
		\r\n\
		if( document.getElementById('n_s_Lan_param_001').value == \"nie\")document.getElementById('n_s_Lan_br_001').disabled=false;\r\n\
		else document.getElementById('n_s_Lan_br_001').disabled = true;\r\n\
	}\r\n\
</script>\r\n\
<script>\r\n\
\r\n\
 function maskujKlawisze(AEvent, AMaska)\r\n\
{\r\n\
 if (window.Event) {\r\n\
  kodKlawisza = AEvent.which;\r\n\
 }\r\n\
\r\n\
 else {\r\n\
   kodKlawisza = AEvent.keyCode;\r\n\
 }\r\n\
 if (kodKlawisza == 13) {\r\n\
  return true;  // Enter\r\n\
 };\r\n\
 if (kodKlawisza == 0) {\r\n\
  return true;  // klawisze sterujące\r\n\
 };\r\n\
 if (kodKlawisza == 8) {\r\n\
  return true;  // backspace\r\n\
 };\r\n\
 klawisz = String.fromCharCode(kodKlawisza);\r\n\
 if (AMaska.indexOf(klawisz) == -1) {\r\n\
  return false;\r\n\
 } else {\r\n\
   return true;\r\n\
 }\r\n\
}\r\n\
 function dop_zakr_min_sek(element){\r\n\
   if(element.value>59){ alert('wartosc powinna miescic sie w przedziale 0 - 59'); element.value='00'; }\r\n\
}\r\n\
\r\n\
</script>\r\n\
 <style> \r\n\
.menu {\r\n\
    border: none;\r\n\
    border: 0px;\r\n\
    margin: 0px;\r\n\
    padding: 0px;\r\n\
    font-family: verdana,geneva,arial,helvetica,sans-serif;\r\n\
    font-size: 18px;\r\n\
    font-weight: bold;\r\n\
    color: 8e8e8e;\r\n\
}\r\n\
.menu li {\r\n\
    float: left;\r\n\
    padding: 0px 0px 0px 0px;\r\n\
}\r\n\
.menu li a {\r\n\
    color: #666666;\r\n\
    display: block;\r\n\
    font-weight: bold;\r\n\
    line-height: 43px;\r\n\
    padding: 0px 15px;\r\n\
    text-align: center;\r\n\
    text-decoration: none;\r\n\
}\r\n\
.menu li a:hover {\r\n\
    color: #000000;\r\n\
    text-decoration: none;\r\n\
}\r\n\
.menu li ul {\r\n\
    background: #e0e0e0;\r\n\
    border-left: 2px solid #a80329;\r\n\
    border-right: 2px solid #a80329;\r\n\
    border-bottom: 2px solid #a80329;\r\n\
    display: none;\r\n\
    height: auto;\r\n\
    filter: alpha(opacity=95);\r\n\
    opacity: 0.95;\r\n\
    position: absolute;\r\n\
    width: 225px;\r\n\
    z-index: 100;\r\n\
    \r\n\
}\r\n\
.menu li:hover ul {\r\n\
    display: block;\r\n\
}\r\n\
.menu li li {\r\n\
   // display: block;\r\n\
    float: none;\r\n\
    padding: 0px;\r\n\
    width: 225px;\r\n\
}\r\n\
.menu li ul a {\r\n\
    display: block;\r\n\
    font-size: 16px;\r\n\
    font-style: normal;\r\n\
    padding: 0px 10px 0px 15px;\r\n\
    text-align: left;\r\n\
}\r\n\
.menu li ul a:hover {\r\n\
    background: #949494;\r\n\
    color: #000000;\r\n\
    opacity: 1.0;\r\n\
    filter: alpha(opacity=100);\r\n\
}\r\n\
.menu #current {\r\n\
    background: url(data:image/gif;base64,R0lGODlhAQArAJEAALk1VHsMJqgDKQAAACH5BAAAAAAALAAAAAABACsAAAIHFISpy+0aCgA7) top left repeat-x;\r\n\
    color: #ffffff;\r\n\
}\r\n\
</style>\r\n\
<style>\r\n\
­iv { margin: 20px; }\r\n\
 #tu_ant {\r\n\
 padding: 15px 12px;\r\n\
 margin-top: 11px;\r\n\
 border-radius: 6px; background:   url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABkAAAAcCAYAAACUJBTQAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAgY0hSTQAAeiYAAICEAAD6AAAAgOgAAHUwAADqYAAAOpgAABdwnLpRPAAAAAlwSFlzAAAOxAAADsQBlSsOGwAABsFJREFUSEvtVl2sVNUV/vb5n3POzL2DXFDEolRQxKqtQmpbQ6Vogj/EmCCmoQkvVvx56F+I7UsfeaFNSlJjK02QqkmLL6Y19aeNQdT6oKA4pEKsjbfQcJF75947d+bM+dl791tnuNHXvndy150ze6+91vrW+tbaRw1OPWAd5QDWhzIelAUcaCiUXKvAfxTuf0GscrnswCgu84BjqaPkjOhq7slvC+MoDLWB4xYKrq4oQ7hVDkeX1KcDEfOFb3k2Fc9rGDqvVAEDii1RMSBZM9SxRpwY6lrYokLiMwivaMKpXLheSOeyqQmAimC0bgRNdFlhUVqFyndRcjmj2Zy6mQQXBFCeyxWForIoyovIec41CWwmIN/9ri2cWXiRT5spytyiKjyUJkYZLMcnk9PofPgxPjo1ifNT8wwCGGtGGG9FWH3lBG77+vVYPtGgiwyRr1EM56GZkUYzBXIJeoFOjm23KmEq/ABTnxVI0+Vw3aV48aX38Jvn/47JczlmZys41A8CoskIn2mQKil4aI47WHNVjJ07N2DrnV/l2QGG+TQznWM8ipn+AZ0c32FtajHVLXHZyhvx0p9P4Bf7XsPp0z1UqYdhxUS4hE4HaeLCdxV0YaAZZTnPvHM/afkYzpe4+sYWnvjZd7Dh1hWI3RyYXUBU5CMnXTIjTFbgt0+/hV8/dQIDHgjGeHCuxM23XYpN316PdeuWYemyCHEjRNY3mLlQ4IP3zjKYKbz8pxM1r7y2D03Gff+xa7B712Y0h9NISkHywYPWtCewd9/LeGr/v+CMkXZDi1tuaWLvnq1YdXkD40tZQLKvn7EmYs5xkFdMmMt1m+DkP85h3/5X8O77Ug9mPlbYtfMm7HnoK2jqWVKcKLK5DG8dOY8oVTDM+U+f+CYOHXgc69YEiNwZZLNTjEYjjZcg9McRuDHarQTNgOQdnsHNN4zj8DMP4yePrkfkMQzaOPLqR2Qr00lRCye2WU+neOPNU3jymX/iwR2r8MD2OzAzPcUmyhGQcZ9MZjjeOY93jk1iQDq30wauvnIJtnxjLZaOGbQYeT7sIRqL8crrHex/8jR2fW8ttm25CqEdQvU622xcNIAwIHzDHmJRS82+CDC94OLAoaM4/OJZzMyxf0aNDsuapolC03Ow/e7LseeH9yEb/AftCYVBPiTado2iyubYbSROr3OvDfseFrICzYkmev0+WmmMf59TeOjHf0HnZB+GlJWPz1wYoT5FPiwNFPe+fG0TB5/ejbHWZ2Rfv15jPEwrFZWhk5P32LgkEk3PHAFKKMp5s28/++TZs0xDRWo3cP+2Dbhj89d4aIBS5zjy9nE8/8fTJImui33f3Zdi78/vRb/7KSbYrGwWZobzzWVfCVsqQgT7wPUUFLuO5cdNN4xBcW3jpkvwh+d24PFHrsH6tV3cuhG4fl0Pux++Fi8cvger1yT17Lx90xVI4hITaQj0B0DFBpfxQPtqoXOX9XsOvFaDUQ1QMKwkCsn3AGe6Bu1lbRTZNC7hftG36HbnsfK6FejPzkK7LrKMA3Posfkc+MUAfp7B80jzmDXmLNNkG51stf6chdcIkLNIUeKzJyzTRBLAZyEcIjK4cKHEoYMdvHFkGhs3jhHJFsTpgB0/QDtJYPolRw87kTVAQCfsdEmXrdNFFnhS0IJdToNlUWB+ro+QtWkwooJE8Dhlf/dchxPhDD7+NMPvnz2HAwePopWMU4eN2Z2jIUmPEEQz/RmMoHFkDtTZ5ANp67AeMofkHkhjnwziuOefUj4y4+DNY3P1iK+SGEPy5Oj7F9CnusMAwkjGJX+4JEFI+nMAD0ODSqaqNKN+53biIUyBWjsWyCIj5xy06JGzXrQMP/jR6/jbX7vYvHUJfvXLzbD9GYS8yFy5Cesak7hixmN0RCG9knkekdR3qGjIRTW6NrVIDVScOYyBWa0MGoGDtOkgDl3kg4JBisXFACUpn4vEJyhkrPC0OBnZE++GoglTRFBo3pLyCqDl+iXCvMc9EiNu8H2AwdHM6GydhtGzAFs0ygzSiaRlUYeohCEuayBSp5k5l6hch0zj6ZBIXJJikPG+l+O11cUoR8+S7UWjmhn8P5L/sSaf12hUNNbIspGsxztc3r3YsQmv4qChsXJVhMtWR7jiSxHzznnncKQIQUSoJ1eFvORZ+S2fEZM5hd/+lo0duSikWheZMnoVucgYeeZkFi5pjhz5SeMOx0fdH7UsqgrlF8/yu6BwhDk554uWJ76mMny2DB0adjwnmxjVJuA7mNwj3JOJWk8I4bQ7WqNufVbmHMXQhqaU5P8wdNBlg/4X8S+jJn4L8/oAAAAASUVORK5CYII=') no-repeat; }\r\n\
 </style>\r\n\
<style>\r\n\
/* H7 cards grow with content instead of reserving fixed empty space. */\r\n\
.corner-radius2,.corner-radius4{height:auto!important;min-height:0;display:flow-root;overflow:visible;padding-bottom:14px;max-width:100%;box-sizing:border-box;}\r\n\
.corner-radius,.corner-radius3:not(.port-column){height:auto!important;min-height:0;overflow:visible;padding:10px 8px;box-sizing:border-box;max-width:calc(100% - 20px);font:16px/1.5 Arial,Helvetica,sans-serif;color:#eee;}\r\n\
.corner-radius .css-font-style,.corner-radius3 .css-font-style{font-size:18px;line-height:1.4;}\r\n\
.corner-radius .css-font-style2,.corner-radius3 .css-font-style2{font-size:16px;line-height:1.5;}\r\n\
.corner-radius table,.corner-radius3 table{font:15px/1.4 Arial,Helvetica,sans-serif;color:#eee;max-width:100%;margin:6px auto;border-spacing:4px;}\r\n\
.corner-radius td,.corner-radius th{padding:3px 2px;vertical-align:middle;}\r\n\
.corner-radius input:not([type=checkbox]):not([type=radio]),.corner-radius select,.corner-radius3 input:not([type=checkbox]):not([type=radio]){max-width:100%;box-sizing:border-box;}\r\n\
.corner-radius .textbox,.corner-radius3 .textbox{height:30px;font-size:15px;}\r\n\
.corner-radius .css_button,.corner-radius3 .css_button{font-size:17px;}\r\n\
.corner-radius .regular-checkbox+label,.corner-radius3 .regular-checkbox+label{vertical-align:middle;}\r\n\
.h7-read-grid{display:flex;flex-wrap:wrap;align-items:flex-start;gap:14px 0;}.h7-read-grid>.y{float:none;padding-bottom:0;}\r\n\
.y{height:auto!important;min-height:0;display:flow-root;overflow:visible;padding-bottom:14px;max-width:100%;}\r\n\
.x{height:auto!important;min-height:110px;font-size:16px;line-height:1.4;padding:10px 4px;box-sizing:border-box;max-width:calc(100% - 10px);overflow:visible;}\r\n\
.x .a{font-size:42px;}.x .b{font-size:17px;}.x .z{font-size:26px;}\r\n\
@media(max-width:600px){.corner-radius2,.corner-radius4,.y{float:none!important;width:100%!important;}.corner-radius,.corner-radius3:not(.port-column),.x{float:none!important;margin-left:auto!important;margin-right:auto!important;}.corner-radius table{font-size:14px;border-spacing:2px;}}\r\n\
</style>\r\n\
</head>";

const char HttpStructMainReadPanel[]="\r\n\
 <script>\r\n\
 var nr_pwm; var val_pwm; var i,j,wst;  var txt=[]; \r\n\
 wst=0; \r\n\
 var now=0;void setInterval(function(){now++;if(now==2){wst=0;}},50);\r\n\
 var pwmBusy=[],pwmPending=[];function loadXMLDoc_PWM()\r\n\
 {var pwmChannel=nr_pwm,pwmValue=val_pwm;if(pwmBusy[pwmChannel]){pwmPending[pwmChannel]=pwmValue;return;}pwmBusy[pwmChannel]=true;var xmlhttp;       \r\n\
 if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
 else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n\
 xmlhttp.onreadystatechange=function(){if(xmlhttp.readyState==4){pwmBusy[pwmChannel]=false;if(pwmPending[pwmChannel]!==undefined){var latest=pwmPending[pwmChannel];delete pwmPending[pwmChannel];nr_pwm=pwmChannel;val_pwm=latest;loadXMLDoc_PWM();}}};xmlhttp.open(\"GET\",\"/lpc.cgi/n_pwm\"+pwmChannel+\"=\"+pwmValue+\"&n_115_zapisz=\", true);\r\n\
 xmlhttp.send();\r\n\
 }\r\n\
 function loadXMLDoc_PK()\r\n\
 { var xmlhttp; var controlPort=nr_pwm;       \r\n\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n\
   xmlhttp.onreadystatechange=function()\r\n\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200){\r\n\
	  txt= xmlhttp.responseText;\r\n\
	  if(txt[0]=='<'){\r\n\
if(controlPort==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==25) document.getElementById(\"tu_pk025\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==26) document.getElementById(\"tu_pk026\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==27) document.getElementById(\"tu_pk027\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==29) document.getElementById(\"tu_pk029\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==30) document.getElementById(\"tu_pk030\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==31) document.getElementById(\"tu_pk031\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==32) document.getElementById(\"tu_pk032\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\r\n\
}\r\n\
}\r\n\
}\r\n\
if(controlPort==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\r\n\
if(controlPort==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\r\n\
if(controlPort==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\r\n\
if(controlPort==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\r\n\
if(controlPort==25) xmlhttp.open(\"GET\",\"/mobile/pk008\", true);\r\n\
if(controlPort==26) xmlhttp.open(\"GET\",\"/mobile/pk009\", true);\r\n\
if(controlPort==27) xmlhttp.open(\"GET\",\"/mobile/pk010\", true);\r\n\
if(controlPort==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\r\n\
if(controlPort==29) xmlhttp.open(\"GET\",\"/mobile/pk002\", true);\r\n\
if(controlPort==30) xmlhttp.open(\"GET\",\"/mobile/pk003\", true);\r\n\
if(controlPort==31) xmlhttp.open(\"GET\",\"/mobile/pk004\", true);\r\n\
if(controlPort==32) xmlhttp.open(\"GET\",\"/mobile/pk005\", true);\r\n\
if(controlPort==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\r\n\
xmlhttp.send();\r\n\
}\r\n\
function xpk021(){nr_pwm=21;loadXMLDoc_PK();}\r\n\
function xpk022(){nr_pwm=22;loadXMLDoc_PK();}\r\n\
function xpk023(){nr_pwm=23;loadXMLDoc_PK();}\r\n\
function xpk024(){nr_pwm=24;loadXMLDoc_PK();}\r\n\
function xpk025(){nr_pwm=25;loadXMLDoc_PK();}\r\n\
function xpk026(){nr_pwm=26;loadXMLDoc_PK();}\r\n\
function xpk027(){nr_pwm=27;loadXMLDoc_PK();}\r\n\
function xpk028(){nr_pwm=28;loadXMLDoc_PK();}\r\n\
function xpk029(){nr_pwm=29;loadXMLDoc_PK();}\r\n\
function xpk030(){nr_pwm=30;loadXMLDoc_PK();}\r\n\
function xpk031(){nr_pwm=31;loadXMLDoc_PK();}\r\n\
function xpk032(){nr_pwm=32;loadXMLDoc_PK();}\r\n\
function xpk033(){nr_pwm=33;loadXMLDoc_PK();}\r\n\
function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){nr_pwm=1;val_pwm=val;loadXMLDoc_PWM();}\r\n\
function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){nr_pwm=2;val_pwm=val;loadXMLDoc_PWM();}\r\n\
function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){nr_pwm=3;val_pwm=val;loadXMLDoc_PWM();}\r\n\
var myVar = setInterval(loadXMLDoc, 1000      );\r\n\
 var txt=[]; \r\n\
 var txt1=[]; \r\n\
 var txt2=[]; \r\n\
 var txt3=[]; \r\n\
 var txt4=[]; \r\n\
 var txt5=[]; \r\n\
 var ccol=[]; \r\n\
 function loadXMLDoc()\r\n\
 {if(wst==0){wst=1;       now=0;\r\n\
   var xmlhttp;\r\n\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n\
   xmlhttp.onreadystatechange=function()\r\n\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200)\r\n\
     {  \r\n\
		 txt= xmlhttp.responseText;\r\n\
		 txt3=txt[0]+txt[1]+txt[2]+txt[3]+txt[4]+txt[5]+txt[6]+txt[7]; document.getElementById(\"tu_zgr\").innerHTML=txt3;\r\n\
		 txt3=\"GSM: \"+txt[23]+txt[24]; document.getElementById(\"tu_gsm\").innerHTML=txt3;\r\n\
if((txt[25+10]=='0')||(txt[25+10]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk021\").innerHTML=txt3; document.getElementById(\"tu_ti021\").innerHTML=txt3; document.getElementById(\"tu_te021\").innerHTML=txt3;   if((txt[25+10]=='2')||(txt[25+10]=='3')) document.getElementById(\"tu_lora021wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora021wy\").innerHTML=\" \";\r\n\
if((txt[25+11]=='0')||(txt[25+11]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk022\").innerHTML=txt3; document.getElementById(\"tu_ti022\").innerHTML=txt3; document.getElementById(\"tu_te022\").innerHTML=txt3;   if((txt[25+11]=='2')||(txt[25+11]=='3')) document.getElementById(\"tu_lora022wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora022wy\").innerHTML=\" \";\r\n\
if((txt[25+12]=='0')||(txt[25+12]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk023\").innerHTML=txt3; document.getElementById(\"tu_ti023\").innerHTML=txt3; document.getElementById(\"tu_te023\").innerHTML=txt3;   if((txt[25+12]=='2')||(txt[25+12]=='3')) document.getElementById(\"tu_lora023wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora023wy\").innerHTML=\" \";\r\n\
if((txt[25+6]=='0')||(txt[25+6]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk024\").innerHTML=txt3; document.getElementById(\"tu_ti024\").innerHTML=txt3; document.getElementById(\"tu_te024\").innerHTML=txt3;   if((txt[25+6]=='2')||(txt[25+6]=='3')) document.getElementById(\"tu_lora024wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora024wy\").innerHTML=\" \";\r\n\
if((txt[25+7]=='0')||(txt[25+7]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk025\").innerHTML=txt3; document.getElementById(\"tu_ti025\").innerHTML=txt3; document.getElementById(\"tu_te025\").innerHTML=txt3;   if((txt[25+7]=='2')||(txt[25+7]=='3')) document.getElementById(\"tu_lora025wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora025wy\").innerHTML=\" \";\r\n\
if((txt[25+8]=='0')||(txt[25+8]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk026\").innerHTML=txt3; document.getElementById(\"tu_ti026\").innerHTML=txt3; document.getElementById(\"tu_te026\").innerHTML=txt3;   if((txt[25+8]=='2')||(txt[25+8]=='3')) document.getElementById(\"tu_lora026wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora026wy\").innerHTML=\" \";\r\n\
if((txt[25+9]=='0')||(txt[25+9]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk027\").innerHTML=txt3; document.getElementById(\"tu_ti027\").innerHTML=txt3; document.getElementById(\"tu_te027\").innerHTML=txt3;   if((txt[25+9]=='2')||(txt[25+9]=='3')) document.getElementById(\"tu_lora027wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora027wy\").innerHTML=\" \";\r\n\
if((txt[25+0]=='0')||(txt[25+0]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk028\").innerHTML=txt3; document.getElementById(\"tu_ti028\").innerHTML=txt3; document.getElementById(\"tu_te028\").innerHTML=txt3;   if((txt[25+0]=='2')||(txt[25+0]=='3')) document.getElementById(\"tu_lora028wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora028wy\").innerHTML=\" \";\r\n\
if((txt[25+1]=='0')||(txt[25+1]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk029\").innerHTML=txt3; document.getElementById(\"tu_ti029\").innerHTML=txt3; document.getElementById(\"tu_te029\").innerHTML=txt3;   if((txt[25+1]=='2')||(txt[25+1]=='3')) document.getElementById(\"tu_lora029wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora029wy\").innerHTML=\" \";\r\n\
if((txt[25+2]=='0')||(txt[25+2]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk030\").innerHTML=txt3; document.getElementById(\"tu_ti030\").innerHTML=txt3; document.getElementById(\"tu_te030\").innerHTML=txt3;   if((txt[25+2]=='2')||(txt[25+2]=='3')) document.getElementById(\"tu_lora030wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora030wy\").innerHTML=\" \";\r\n\
if((txt[25+3]=='0')||(txt[25+3]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk031\").innerHTML=txt3; document.getElementById(\"tu_ti031\").innerHTML=txt3; document.getElementById(\"tu_te031\").innerHTML=txt3;   if((txt[25+3]=='2')||(txt[25+3]=='3')) document.getElementById(\"tu_lora031wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora031wy\").innerHTML=\" \";\r\n\
if((txt[25+4]=='0')||(txt[25+4]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk032\").innerHTML=txt3; document.getElementById(\"tu_ti032\").innerHTML=txt3; document.getElementById(\"tu_te032\").innerHTML=txt3;   if((txt[25+4]=='2')||(txt[25+4]=='3')) document.getElementById(\"tu_lora032wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora032wy\").innerHTML=\" \";\r\n\
if((txt[25+5]=='0')||(txt[25+5]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk033\").innerHTML=txt3; document.getElementById(\"tu_ti033\").innerHTML=txt3; document.getElementById(\"tu_te033\").innerHTML=txt3;   if((txt[25+5]=='2')||(txt[25+5]=='3')) document.getElementById(\"tu_lora033wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora033wy\").innerHTML=\" \";\r\n\
\r\n\
if((txt[25+13+1+0]=='0')||(txt[25+13+1+0]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we001\").innerHTML=txt3; if((txt[25+13+1+0]=='2')||(txt[25+13+1+0]=='3')) document.getElementById(\"tu_lora001we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+1]=='0')||(txt[25+13+1+1]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we002\").innerHTML=txt3; if((txt[25+13+1+1]=='2')||(txt[25+13+1+1]=='3')) document.getElementById(\"tu_lora002we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+2]=='0')||(txt[25+13+1+2]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we003\").innerHTML=txt3; if((txt[25+13+1+2]=='2')||(txt[25+13+1+2]=='3')) document.getElementById(\"tu_lora003we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+3]=='0')||(txt[25+13+1+3]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we004\").innerHTML=txt3; if((txt[25+13+1+3]=='2')||(txt[25+13+1+3]=='3')) document.getElementById(\"tu_lora004we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+4]=='0')||(txt[25+13+1+4]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we005\").innerHTML=txt3; if((txt[25+13+1+4]=='2')||(txt[25+13+1+4]=='3')) document.getElementById(\"tu_lora005we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+5]=='0')||(txt[25+13+1+5]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we006\").innerHTML=txt3; if((txt[25+13+1+5]=='2')||(txt[25+13+1+5]=='3')) document.getElementById(\"tu_lora006we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+6]=='0')||(txt[25+13+1+6]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we007\").innerHTML=txt3; if((txt[25+13+1+6]=='2')||(txt[25+13+1+6]=='3')) document.getElementById(\"tu_lora007we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+7]=='0')||(txt[25+13+1+7]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we008\").innerHTML=txt3; if((txt[25+13+1+7]=='2')||(txt[25+13+1+7]=='3')) document.getElementById(\"tu_lora008we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+8]=='0')||(txt[25+13+1+8]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we009\").innerHTML=txt3; if((txt[25+13+1+8]=='2')||(txt[25+13+1+8]=='3')) document.getElementById(\"tu_lora009we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+9]=='0')||(txt[25+13+1+9]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we010\").innerHTML=txt3; if((txt[25+13+1+9]=='2')||(txt[25+13+1+9]=='3')) document.getElementById(\"tu_lora010we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+10]=='0')||(txt[25+13+1+10]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we011\").innerHTML=txt3; if((txt[25+13+1+10]=='2')||(txt[25+13+1+10]=='3')) document.getElementById(\"tu_lora011we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+11]=='0')||(txt[25+13+1+11]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we012\").innerHTML=txt3; if((txt[25+13+1+11]=='2')||(txt[25+13+1+11]=='3')) document.getElementById(\"tu_lora012we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+12]=='0')||(txt[25+13+1+12]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we013\").innerHTML=txt3; if((txt[25+13+1+12]=='2')||(txt[25+13+1+12]=='3')) document.getElementById(\"tu_lora013we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+13]=='0')||(txt[25+13+1+13]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we014\").innerHTML=txt3; if((txt[25+13+1+13]=='2')||(txt[25+13+1+13]=='3')) document.getElementById(\"tu_lora014we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+14]=='0')||(txt[25+13+1+14]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we017\").innerHTML=txt3; if((txt[25+13+1+14]=='2')||(txt[25+13+1+14]=='3')) document.getElementById(\"tu_lora017we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+15]=='0')||(txt[25+13+1+15]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we018\").innerHTML=txt3; if((txt[25+13+1+15]=='2')||(txt[25+13+1+15]=='3')) document.getElementById(\"tu_lora018we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+16]=='0')||(txt[25+13+1+16]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we019\").innerHTML=txt3; if((txt[25+13+1+16]=='2')||(txt[25+13+1+16]=='3')) document.getElementById(\"tu_lora019we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+17]=='0')||(txt[25+13+1+17]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we020\").innerHTML=txt3; if((txt[25+13+1+17]=='2')||(txt[25+13+1+17]=='3')) document.getElementById(\"tu_lora020we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020we\").innerHTML=\" \";\r\n\
\r\n\
if((txt[25+13+1+18+1+0]=='5')||(txt[25+13+1+18+1+0]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+0]=='0')||(txt[25+13+1+18+1+0]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol001\").innerHTML=txt3; if((txt[25+13+1+18+1+0]=='2')||(txt[25+13+1+18+1+0]=='3')) document.getElementById(\"tu_lora001eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+1]=='5')||(txt[25+13+1+18+1+1]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+1]=='0')||(txt[25+13+1+18+1+1]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol002\").innerHTML=txt3; if((txt[25+13+1+18+1+1]=='2')||(txt[25+13+1+18+1+1]=='3')) document.getElementById(\"tu_lora002eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+2]=='5')||(txt[25+13+1+18+1+2]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+2]=='0')||(txt[25+13+1+18+1+2]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol003\").innerHTML=txt3; if((txt[25+13+1+18+1+2]=='2')||(txt[25+13+1+18+1+2]=='3')) document.getElementById(\"tu_lora003eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+3]=='5')||(txt[25+13+1+18+1+3]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+3]=='0')||(txt[25+13+1+18+1+3]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol004\").innerHTML=txt3; if((txt[25+13+1+18+1+3]=='2')||(txt[25+13+1+18+1+3]=='3')) document.getElementById(\"tu_lora004eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+4]=='5')||(txt[25+13+1+18+1+4]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+4]=='0')||(txt[25+13+1+18+1+4]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol005\").innerHTML=txt3; if((txt[25+13+1+18+1+4]=='2')||(txt[25+13+1+18+1+4]=='3')) document.getElementById(\"tu_lora005eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+5]=='5')||(txt[25+13+1+18+1+5]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+5]=='0')||(txt[25+13+1+18+1+5]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol006\").innerHTML=txt3; if((txt[25+13+1+18+1+5]=='2')||(txt[25+13+1+18+1+5]=='3')) document.getElementById(\"tu_lora006eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+6]=='5')||(txt[25+13+1+18+1+6]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+6]=='0')||(txt[25+13+1+18+1+6]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol007\").innerHTML=txt3; if((txt[25+13+1+18+1+6]=='2')||(txt[25+13+1+18+1+6]=='3')) document.getElementById(\"tu_lora007eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+7]=='5')||(txt[25+13+1+18+1+7]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+7]=='0')||(txt[25+13+1+18+1+7]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol008\").innerHTML=txt3; if((txt[25+13+1+18+1+7]=='2')||(txt[25+13+1+18+1+7]=='3')) document.getElementById(\"tu_lora008eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+8]=='5')||(txt[25+13+1+18+1+8]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+8]=='0')||(txt[25+13+1+18+1+8]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol009\").innerHTML=txt3; if((txt[25+13+1+18+1+8]=='2')||(txt[25+13+1+18+1+8]=='3')) document.getElementById(\"tu_lora009eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+9]=='5')||(txt[25+13+1+18+1+9]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+9]=='0')||(txt[25+13+1+18+1+9]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol010\").innerHTML=txt3; if((txt[25+13+1+18+1+9]=='2')||(txt[25+13+1+18+1+9]=='3')) document.getElementById(\"tu_lora010eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+10]=='5')||(txt[25+13+1+18+1+10]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+10]=='0')||(txt[25+13+1+18+1+10]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol011\").innerHTML=txt3; if((txt[25+13+1+18+1+10]=='2')||(txt[25+13+1+18+1+10]=='3')) document.getElementById(\"tu_lora011eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+11]=='5')||(txt[25+13+1+18+1+11]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+11]=='0')||(txt[25+13+1+18+1+11]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol012\").innerHTML=txt3; if((txt[25+13+1+18+1+11]=='2')||(txt[25+13+1+18+1+11]=='3')) document.getElementById(\"tu_lora012eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+12]=='5')||(txt[25+13+1+18+1+12]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+12]=='0')||(txt[25+13+1+18+1+12]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol013\").innerHTML=txt3; if((txt[25+13+1+18+1+12]=='2')||(txt[25+13+1+18+1+12]=='3')) document.getElementById(\"tu_lora013eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+13]=='5')||(txt[25+13+1+18+1+13]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+13]=='0')||(txt[25+13+1+18+1+13]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol014\").innerHTML=txt3; if((txt[25+13+1+18+1+13]=='2')||(txt[25+13+1+18+1+13]=='3')) document.getElementById(\"tu_lora014eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014eol\").innerHTML=\" \";\r\n\
\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp001a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora001a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp001b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora001b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+7+i];txt4[5]=txt[25+13+1+18+1+14+1+0+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+0+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp001c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp002a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora002a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp002b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora002b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+7+i];txt4[5]=txt[25+13+1+18+1+14+1+12+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+12+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp002c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp003a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora003a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp003b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora003b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+7+i];txt4[5]=txt[25+13+1+18+1+14+1+24+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+24+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp003c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp004a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora004a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp004b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora004b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+7+i];txt4[5]=txt[25+13+1+18+1+14+1+36+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+36+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp004c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+7+i];txt4[5]=txt[25+13+1+18+1+14+1+48+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+48+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+7+i];txt4[5]=txt[25+13+1+18+1+14+1+60+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+60+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+7+i];txt4[5]=txt[25+13+1+18+1+14+1+72+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+72+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+7+i];txt4[5]=txt[25+13+1+18+1+14+1+84+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+84+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+7+i];txt4[5]=txt[25+13+1+18+1+14+1+96+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+96+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+7+i];txt4[5]=txt[25+13+1+18+1+14+1+108+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+108+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp011a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora011a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp011b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora011b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+7+i];txt4[5]=txt[25+13+1+18+1+14+1+120+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+120+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp011c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp012a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora012a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp012b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora012b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+7+i];txt4[5]=txt[25+13+1+18+1+14+1+132+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+132+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp012c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp013a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora013a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp013b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora013b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+7+i];txt4[5]=txt[25+13+1+18+1+14+1+144+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+144+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp013c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp014a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora014a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp014b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora014b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+7+i];txt4[5]=txt[25+13+1+18+1+14+1+156+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+156+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp014c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp017a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora017a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp017b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora017b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+7+i];txt4[5]=txt[25+13+1+18+1+14+1+168+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+168+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp017c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp018a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora018a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp018b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora018b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+7+i];txt4[5]=txt[25+13+1+18+1+14+1+180+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+180+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp018c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp019a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora019a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp019b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora019b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+7+i];txt4[5]=txt[25+13+1+18+1+14+1+192+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+192+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp019c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp020a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora020a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp020b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora020b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+7+i];txt4[5]=txt[25+13+1+18+1+14+1+204+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+204+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp020c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
 for(i=0;i<5;i++) txt5[i]=txt[25+13+1+18+1+14+1+216+1+20+1+140+1+0+i];for(i=0;i<9;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+0+i];document.getElementById(\"tu_adc015\").innerHTML=txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" \"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4];\r\n\
for(i=0;i<5;i++) txt5[i]=txt[25+13+1+18+1+14+1+216+1+20+1+140+1+6+i];for(i=0;i<9;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+10+i];document.getElementById(\"tu_adc016\").innerHTML=txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" \"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4];\r\n\
\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+0+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt001\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+10+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt002\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+20+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt003\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+30+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt004\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+40+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt005\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+50+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt006\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+60+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt007\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+70+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt008\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+80+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt009\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+90+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt010\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+100+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt011\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+110+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt012\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+120+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt013\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+130+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt014\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_017\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+\"<font color='#FFF' size='6'>\"+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+18*0+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp1_017\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+18*1+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp2_017\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+18*2+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp3_017\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+18*3+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp4_017\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_018\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+\"<font color='#FFF' size='6'>\"+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+18*0+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp1_018\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+18*1+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp2_018\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+18*2+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp3_018\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+18*3+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp4_018\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_019\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+\"<font color='#FFF' size='6'>\"+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+18*0+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp1_019\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+18*1+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp2_019\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+18*2+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp3_019\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+18*3+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp4_019\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_020\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+\"<font color='#FFF' size='6'>\"+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+18*0+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp1_020\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+18*1+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp2_020\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+18*2+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp3_020\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
j=0;for(i=0;i<17;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+18*3+23+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;} if(txt5[15]=='1') ccol=\"f88\"; else ccol=\"eee\"; document.getElementById(\"tu_imp4_020\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+\"</font>\";\r\n\
 }\r\n\
 }\r\n\
 xmlhttp.open(\"GET\",\"/TME.txt\", true);\r\n\
 xmlhttp.send();\r\n\
 }}\r\n\
 </script> \r\n\
    <script>\r\n\
  var tyt=[];   tyt[0]=\"<li><a href='/nowa'>Panel graficzny LPC</a></li><li><a href='main'>Panel odczytowy</a></li><li><a href='3main'>Konfiguracja portow</a></li><li><a href='lan'>Ustawienia sieci LAN</a></li><li><a href='wifi'>Ustawienia sieci WiFi</a></li><li><a href='gsm'>Ustawienia sieci GSM</a></li><li><a href='czas'>Ustawienia czasu</a></li><li><a href='maskisms'>Numery telefonow</a></li><li><a href='maskimail'>Skrzynki e-mail</a></li><li><a href='/nrf'>Ustawienia LoRa</a></li><li><a href='logic'>Operacje logiczne</a></li><li><a href='rej'>Rejetrator</a></li><li><a href='Rot'>Rotacja</a></li><li><a href='/wykres1'>Wykres pomiarow</a></li><li><a href='/wykres2'>Wykres licznikow</a></li>\";\r\n\
  document.getElementById(\"tu_menu\").innerHTML=tyt;\r\n\
  </script>\r\n\
</body>\r\n\
\r\n\
  \r\n\
</html>";


const char HttpMainMenu[]="\r\n\
<body bgcolor=\"000000\">\r\n\
\r\n\
<table><tr><td>\r\n\
	 <div class='menu'>\r\n\
	 <ul>\r\n\
	<li><a href='#' id='current'>Menu&nbsp;&#9660;</a>\r\n\
	   <ul>\r\n\
	    <div id=\"tu_menu\"> </div>\r\n\
        <br><font size=\"1\" color='#777'>Nr ident. &nbsp;&nbsp;1122334455667788</font>\r\n\
	   </ul>\r\n\
	  </li>\r\n\
	<br>\r\n\
	</ul>\r\n\
	</div>\r\n\
	<br>\r\n\
	</td>\r\n\
	<td width=\"10px\"></td>\r\n\
    <td><button class=\"n\"><div id=\"tu_zgr\"> Trwa analiza danych ... </div></button></td>\r\n\
	<td width=\"10px\"></td>\r\n\
	<td><button class=\"n\"><div id=\"tu_gsm\"> -- </div></button></td>\r\n\
	<td width=\"10px\"></td>\r\n\
	<td>                                                                           </td>\r\n\
	</tr></table>\r\n\
\r\n\
";

const char HttpMainReadPanel[]="\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_temp001a\"><font color='#eee'>   --.- &deg;C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div><div id=\"tu_lora001a\"></div>\r\n\
<div id=\"tu_temp001b\" style=\"display: none;\"></div><div id=\"tu_lora001c\"></div><div id=\"tu_temp001c\" style=\"display: none;\"></div><div id=\"tu_lora001b\"></div><div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_temp002a\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div><div id=\"tu_lora002a\"></div>\r\n\
<div id=\"tu_temp002b\" style=\"display: none;\"></div><div id=\"tu_lora002c\"></div><div id=\"tu_temp002c\" style=\"display: none;\"></div><div id=\"tu_lora002b\"></div><div id=\"tu_temp003a\" style=\"display: none;\"></div><div id=\"tu_lora003a\"></div><div id=\"tu_temp003b\" style=\"display: none;\"></div><div id=\"tu_lora003c\"></div><div id=\"tu_temp003c\" style=\"display: none;\"></div><div id=\"tu_lora003b\"></div><div id=\"tu_temp004a\" style=\"display: none;\"></div><div id=\"tu_lora004a\"></div><div id=\"tu_temp004b\" style=\"display: none;\"></div><div id=\"tu_lora004c\"></div><div id=\"tu_temp004c\" style=\"display: none;\"></div><div id=\"tu_lora004b\"></div><div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\"><font class=\"a\"><b><div id=\"tu_temp005b\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora005c\"></div><br>\r\n\
<font class=\"a\"><b><div id=\"tu_temp005c\"><font color='#eee'>  --   %</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora005b\"></div></div></div>\r\n\
<div id=\"tu_temp005a\" style=\"display: none;\"></div><div id=\"tu_lora005a\"></div><div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\"><font class=\"a\"><b><div id=\"tu_temp006b\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora006c\"></div><br>\r\n\
<font class=\"a\"><b><div id=\"tu_temp006c\"><font color='#eee'>  --   %</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora006b\"></div></div></div>\r\n\
<div id=\"tu_temp006a\" style=\"display: none;\"></div><div id=\"tu_lora006a\"></div><div id=\"tu_temp007a\" style=\"display: none;\"></div><div id=\"tu_lora007a\"></div><div id=\"tu_temp007b\" style=\"display: none;\"></div><div id=\"tu_lora007c\"></div><div id=\"tu_temp007c\" style=\"display: none;\"></div><div id=\"tu_lora007b\"></div><div id=\"tu_temp008a\" style=\"display: none;\"></div><div id=\"tu_lora008a\"></div><div id=\"tu_temp008b\" style=\"display: none;\"></div><div id=\"tu_lora008c\"></div><div id=\"tu_temp008c\" style=\"display: none;\"></div><div id=\"tu_lora008b\"></div><div id=\"tu_temp009a\" style=\"display: none;\"></div><div id=\"tu_lora009a\"></div><div id=\"tu_temp009b\" style=\"display: none;\"></div><div id=\"tu_lora009c\"></div><div id=\"tu_temp009c\" style=\"display: none;\"></div><div id=\"tu_lora009b\"></div><div id=\"tu_temp010a\" style=\"display: none;\"></div><div id=\"tu_lora010a\"></div><div id=\"tu_temp010b\" style=\"display: none;\"></div><div id=\"tu_lora010c\"></div><div id=\"tu_temp010c\" style=\"display: none;\"></div><div id=\"tu_lora010b\"></div><div id=\"tu_temp011a\" style=\"display: none;\"></div><div id=\"tu_lora011a\"></div><div id=\"tu_temp011b\" style=\"display: none;\"></div><div id=\"tu_lora011c\"></div><div id=\"tu_temp011c\" style=\"display: none;\"></div><div id=\"tu_lora011b\"></div><div id=\"tu_temp012a\" style=\"display: none;\"></div><div id=\"tu_lora012a\"></div><div id=\"tu_temp012b\" style=\"display: none;\"></div><div id=\"tu_lora012c\"></div><div id=\"tu_temp012c\" style=\"display: none;\"></div><div id=\"tu_lora012b\"></div><div id=\"tu_temp013a\" style=\"display: none;\"></div><div id=\"tu_lora013a\"></div><div id=\"tu_temp013b\" style=\"display: none;\"></div><div id=\"tu_lora013c\"></div><div id=\"tu_temp013c\" style=\"display: none;\"></div><div id=\"tu_lora013b\"></div><div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_temp014a\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div><div id=\"tu_lora014a\"></div>\r\n\
<div id=\"tu_temp014b\" style=\"display: none;\"></div><div id=\"tu_lora014c\"></div><div id=\"tu_temp014c\" style=\"display: none;\"></div><div id=\"tu_lora014b\"></div><div id=\"tu_temp017a\" style=\"display: none;\"></div><div id=\"tu_lora017a\"></div><div id=\"tu_temp017b\" style=\"display: none;\"></div><div id=\"tu_lora017c\"></div><div id=\"tu_temp017c\" style=\"display: none;\"></div><div id=\"tu_lora017b\"></div><div id=\"tu_temp018a\" style=\"display: none;\"></div><div id=\"tu_lora018a\"></div><div id=\"tu_temp018b\" style=\"display: none;\"></div><div id=\"tu_lora018c\"></div><div id=\"tu_temp018c\" style=\"display: none;\"></div><div id=\"tu_lora018b\"></div><div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_temp019a\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div><div id=\"tu_lora019a\"></div>\r\n\
<div id=\"tu_temp019b\" style=\"display: none;\"></div><div id=\"tu_lora019c\"></div><div id=\"tu_temp019c\" style=\"display: none;\"></div><div id=\"tu_lora019b\"></div><div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\"><font class=\"a\"><b><div id=\"tu_temp020b\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora020c\"></div><br>\r\n\
<font class=\"a\"><b><div id=\"tu_temp020c\"><font color='#eee'>  --   %</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora020b\"></div></div></div>\r\n\
<div id=\"tu_temp020a\" style=\"display: none;\"></div><div id=\"tu_lora020a\"></div><div class=\"y\" style=\"height:330px\"><div class=\"x\" style=\"height:300px\"><font class=\"z\" style=\"font-size: 43px;\"><b><div id=\"tu_imp0_017\">         0.000   KWh       </div></font></b><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp1_017\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;&nbsp;&nbsp;godzinny</font><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp2_017\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;dobowy</font><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp3_017\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;tygodniowy</font><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp4_017\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;miesieczny</font><br><font class=\"b\">Nazwa portu nr 17     E1</font></div></div>\r\n\
<div class=\"y\" style=\"height:330px\"><div class=\"x\" style=\"height:300px\"><font class=\"z\" style=\"font-size: 43px;\"><b><div id=\"tu_imp0_018\">         0.000   KWh       </div></font></b><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp1_018\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;&nbsp;&nbsp;godzinny</font><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp2_018\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;dobowy</font><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp3_018\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;tygodniowy</font><br><font class=\"z\" style=\"color:#ccc\"><div id=\"tu_imp4_018\">         0.000  </div></font><font color=\"#ccc\" size=\"4\"> &nbsp;&nbsp;miesieczny</font><br><font class=\"b\">Nazwa portu nr 18     E2</font></div></div>\r\n\
<div id=\"tu_imp0_019\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp1_019\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp2_019\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp3_019\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp4_019\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp0_020\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp1_020\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp2_020\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp3_020\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_imp4_020\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_we001\" style=\"display: none;\"></div><div id=\"tu_lora001we\"></div>\r\n\
<div id=\"tu_we002\" style=\"display: none;\"></div><div id=\"tu_lora002we\"></div>\r\n\
<div id=\"tu_we003\" style=\"display: none;\"></div><div id=\"tu_lora003we\"></div>\r\n\
<div id=\"tu_we004\" style=\"display: none;\"></div><div id=\"tu_lora004we\"></div>\r\n\
<div id=\"tu_we005\" style=\"display: none;\"></div><div id=\"tu_lora005we\"></div>\r\n\
<div id=\"tu_we006\" style=\"display: none;\"></div><div id=\"tu_lora006we\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_we007\"><font color='#eee'>Stan OFF</font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 7</font></div></div><div id=\"tu_lora007we\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_we008\"><font color='#eee'>Stan OFF</font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 8</font></div></div><div id=\"tu_lora008we\"></div>\r\n\
<div id=\"tu_we009\" style=\"display: none;\"></div><div id=\"tu_lora009we\"></div>\r\n\
<div id=\"tu_we010\" style=\"display: none;\"></div><div id=\"tu_lora010we\"></div>\r\n\
<div id=\"tu_we011\" style=\"display: none;\"></div><div id=\"tu_lora011we\"></div>\r\n\
<div id=\"tu_we012\" style=\"display: none;\"></div><div id=\"tu_lora012we\"></div>\r\n\
<div id=\"tu_we013\" style=\"display: none;\"></div><div id=\"tu_lora013we\"></div>\r\n\
<div id=\"tu_we014\" style=\"display: none;\"></div><div id=\"tu_lora014we\"></div>\r\n\
<div id=\"tu_we017\" style=\"display: none;\"></div><div id=\"tu_lora017we\"></div>\r\n\
<div id=\"tu_we018\" style=\"display: none;\"></div><div id=\"tu_lora018we\"></div>\r\n\
<div id=\"tu_we019\" style=\"display: none;\"></div><div id=\"tu_lora019we\"></div>\r\n\
<div id=\"tu_we020\" style=\"display: none;\"></div><div id=\"tu_lora020we\"></div>\r\n\
<div id=\"tu_eol001\" style=\"display: none;\"></div><div id=\"tu_lora001eol\"></div>\r\n\
<div id=\"tu_eol002\" style=\"display: none;\"></div><div id=\"tu_lora002eol\"></div>\r\n\
<div id=\"tu_eol003\" style=\"display: none;\"></div><div id=\"tu_lora003eol\"></div>\r\n\
<div id=\"tu_eol004\" style=\"display: none;\"></div><div id=\"tu_lora004eol\"></div>\r\n\
<div id=\"tu_eol005\" style=\"display: none;\"></div><div id=\"tu_lora005eol\"></div>\r\n\
<div id=\"tu_eol006\" style=\"display: none;\"></div><div id=\"tu_lora006eol\"></div>\r\n\
<div id=\"tu_eol007\" style=\"display: none;\"></div><div id=\"tu_lora007eol\"></div>\r\n\
<div id=\"tu_eol008\" style=\"display: none;\"></div><div id=\"tu_lora008eol\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_eol009\"><font color='#eee'>Stan OFF</font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 9</font></div></div><div id=\"tu_lora009eol\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_eol010\"><font color='#eee'>Stan OFF</font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 10</font></div></div><div id=\"tu_lora010eol\"></div>\r\n\
<div id=\"tu_eol011\" style=\"display: none;\"></div><div id=\"tu_lora011eol\"></div>\r\n\
<div id=\"tu_eol012\" style=\"display: none;\"></div><div id=\"tu_lora012eol\"></div>\r\n\
<div id=\"tu_eol013\" style=\"display: none;\"></div><div id=\"tu_lora013eol\"></div>\r\n\
<div id=\"tu_eol014\" style=\"display: none;\"></div><div id=\"tu_lora014eol\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_adc015\">8.8 V</div></b></font><br><font class=\"b\">Nazwa portu nr 15     0-10V</font></div></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_adc016\">8.4 V</div></b></font><br><font class=\"b\">Nazwa portu nr 16     0-10V</font></div></div>\r\n\
<div id=\"tu_ptt001\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt002\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_ptt003\"> --- °C</div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_ptt004\"> --- °C</div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div>\r\n\
<div id=\"tu_ptt005\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt006\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt007\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt008\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt009\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt010\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt011\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt012\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt013\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt014\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk021(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk021\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR,PWM</font></div></div><div id=\"tu_lora021wy\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk022(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk022\"><font color=\"#fff\">Stan ON</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR,PWM</font></div></div><div id=\"tu_lora022wy\"></div>\r\n\
<div id=\"tu_pk023\" style=\"display: none;\"></div><div id=\"tu_lora023wy\"></div>\r\n\
<div id=\"tu_pk024\" style=\"display: none;\"></div><div id=\"tu_lora024wy\"></div>\r\n\
<div id=\"tu_pk025\" style=\"display: none;\"></div><div id=\"tu_lora025wy\"></div>\r\n\
<div id=\"tu_pk026\" style=\"display: none;\"></div><div id=\"tu_lora026wy\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk027(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk027\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR</font></div></div><div id=\"tu_lora027wy\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk028(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk028\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie Przekaznikowe</font></div></div><div id=\"tu_lora028wy\"></div>\r\n\
<div id=\"tu_pk029\" style=\"display: none;\"></div><div id=\"tu_lora029wy\"></div>\r\n\
<div id=\"tu_pk030\" style=\"display: none;\"></div><div id=\"tu_lora030wy\"></div>\r\n\
<div id=\"tu_pk031\" style=\"display: none;\"></div><div id=\"tu_lora031wy\"></div>\r\n\
<div id=\"tu_pk032\" style=\"display: none;\"></div><div id=\"tu_lora032wy\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk033(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk033\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie Przekaznikowe</font></div></div><div id=\"tu_lora033wy\"></div>\r\n\
<div id=\"tu_ti021\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti022\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti023\"><font color=\"#fff\">Stan ON</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR,PWM</font></div></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti024\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR</font></div></div>\r\n\
<div id=\"tu_ti025\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti026\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti027\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti028\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti029\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Moj przekaznik :)</font></div></div>\r\n\
<div id=\"tu_ti030\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti031\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti032\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti033\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te021\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te022\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te023\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te024\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te025\"><font color=\"#fff\">Stan ON</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR</font></div></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te026\"><font color=\"#fff\">Stan ON</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR</font></div></div>\r\n\
<div id=\"tu_te027\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te028\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te029\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te030\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie Przekaznikowe</font></div></div>\r\n\
<div id=\"tu_te031\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te032\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te033\" style=\"display: none;\"></div>\r\n\
</div></div>\r\n\
</div>\r\n\
";

const char HttpMainReadPanel2[]="\r\n\
<body bgcolor=\"000000\">\r\n\
\r\n\
<table><tr><td>\r\n\
	 <div class='menu'>\r\n\
	 <ul>\r\n\
	<li><a href='#' id='current'>Menu&nbsp;&#9660;</a>\r\n\
	   <ul>\r\n\
	    <div id=\"tu_menu\"> </div>\r\n\
        <br><font size=\"1\" color='#777'>Nr ident. &nbsp;&nbsp;1212121212121212</font>\r\n\
	   </ul>\r\n\
	  </li>\r\n\
	<br>\r\n\
	</ul>\r\n\
	</div>\r\n\
	<br>\r\n\
	</td>\r\n\
	<td width=\"10px\"></td>\r\n\
    <td><button class=\"n\"><div id=\"tu_zgr\"> Trwa analiza danych ... </div></button></td>\r\n\
	<td width=\"10px\"></td>\r\n\
	<td><button class=\"n\"><div id=\"tu_gsm\"> -- </div></button></td>\r\n\
	<td width=\"10px\"></td>\r\n\
	<td>                                                                           </td>\r\n\
	</tr></table>\r\n\
\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_temp005a\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div><div id=\"tu_lora005a\"></div>\r\n\
<div id=\"tu_temp005b\" style=\"display: none;\"></div><div id=\"tu_lora005c\"></div><div id=\"tu_temp005c\" style=\"display: none;\"></div><div id=\"tu_lora005b\"></div><div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\"><font class=\"a\"><b><div id=\"tu_temp006b\"><font color='#eee'>   --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora006c\"></div><br>\r\n\
<font class=\"a\"><b><div id=\"tu_temp006c\"><font color='#eee'>  --   %</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora006b\"></div></div></div>\r\n\
<div id=\"tu_temp006a\" style=\"display: none;\"></div><div id=\"tu_lora006a\"></div><div id=\"tu_temp007a\" style=\"display: none;\"></div><div id=\"tu_lora007a\"></div><div id=\"tu_temp007b\" style=\"display: none;\"></div><div id=\"tu_lora007c\"></div><div id=\"tu_temp007c\" style=\"display: none;\"></div><div id=\"tu_lora007b\"></div><div id=\"tu_temp008a\" style=\"display: none;\"></div><div id=\"tu_lora008a\"></div><div id=\"tu_temp008b\" style=\"display: none;\"></div><div id=\"tu_lora008c\"></div><div id=\"tu_temp008c\" style=\"display: none;\"></div><div id=\"tu_lora008b\"></div><div id=\"tu_temp009a\" style=\"display: none;\"></div><div id=\"tu_lora009a\"></div><div id=\"tu_temp009b\" style=\"display: none;\"></div><div id=\"tu_lora009c\"></div><div id=\"tu_temp009c\" style=\"display: none;\"></div><div id=\"tu_lora009b\"></div><div id=\"tu_temp010a\" style=\"display: none;\"></div><div id=\"tu_lora010a\"></div><div id=\"tu_temp010b\" style=\"display: none;\"></div><div id=\"tu_lora010c\"></div><div id=\"tu_temp010c\" style=\"display: none;\"></div><div id=\"tu_lora010b\"></div>   <div id=\"tu_we005\" style=\"display: none;\"></div><div id=\"tu_lora005we\"></div>\r\n\
<div id=\"tu_we006\" style=\"display: none;\"></div><div id=\"tu_lora006we\"></div>\r\n\
<div id=\"tu_we007\" style=\"display: none;\"></div><div id=\"tu_lora007we\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_we008\"><font color='#eee'>Stan OFF</font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 8</font></div></div><div id=\"tu_lora008we\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_we009\"><font color='#eee'>Stan OFF</font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 9</font></div></div><div id=\"tu_lora009we\"></div>\r\n\
<div id=\"tu_we010\" style=\"display: none;\"></div><div id=\"tu_lora010we\"></div>\r\n\
<div id=\"tu_eol005\" style=\"display: none;\"></div><div id=\"tu_lora005eol\"></div>\r\n\
<div id=\"tu_eol006\" style=\"display: none;\"></div><div id=\"tu_lora006eol\"></div>\r\n\
<div id=\"tu_eol007\" style=\"display: none;\"></div><div id=\"tu_lora007eol\"></div>\r\n\
<div id=\"tu_eol008\" style=\"display: none;\"></div><div id=\"tu_lora008eol\"></div>\r\n\
<div id=\"tu_eol009\" style=\"display: none;\"></div><div id=\"tu_lora009eol\"></div>\r\n\
<div id=\"tu_eol010\" style=\"display: none;\"></div><div id=\"tu_lora010eol\"></div>\r\n\
<div id=\"tu_ptt005\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt006\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_ptt007\"> --- °C</div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div>\r\n\
<div id=\"tu_ptt008\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt009\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ptt010\" style=\"display: none;\"></div>\r\n\
        <div class=\"y\"><div class=\"x\"><table><tr><td width=\"100px\"><output  class=\"v\" style=\"font-size:36px;\" id=\"volume022\">58</output></td><td></td><td><input type=\"range\" name=\"n_pwm2\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"58\"   oninput=\"outputUpdate022(value)\" onchange=\"x022(this.value);\"></input></td></tr></table><font class=\"b\">Nazwa portu nr 22     PW2</font></div></div>\r\n\
<div class=\"y\"><div class=\"x\"><table><tr><td width=\"100px\"><output  class=\"v\" style=\"font-size:36px;\" id=\"volume023\">69</output></td><td></td><td><input type=\"range\" name=\"n_pwm3\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"69\"   oninput=\"outputUpdate023(value)\" onchange=\"x023(this.value);\"></input></td></tr></table><font class=\"b\">Nazwa portu nr 23     PW3</font></div></div>\r\n\
  <div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk028(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk028\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie Przekaznikowe</font></div></div><div id=\"tu_lora028wy\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a href=\"#\" rel=\"external\" onClick=\"xpk021(); this.href='javascript:return false;';\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk021\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR,PWM</font></div></div><div id=\"tu_lora021wy\"></div>\r\n\
<div id=\"tu_pk022\" style=\"display: none;\"></div><div id=\"tu_lora022wy\"></div>\r\n\
<div id=\"tu_pk023\" style=\"display: none;\"></div><div id=\"tu_lora023wy\"></div>\r\n\
<div id=\"tu_pk024\" style=\"display: none;\"></div><div id=\"tu_lora024wy\"></div>\r\n\
<div id=\"tu_pk033\" style=\"display: none;\"></div><div id=\"tu_lora033wy\"></div>\r\n\
<div id=\"tu_ti028\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti021\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti022\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_ti023\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti024\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Timer</font></div></div>\r\n\
<div id=\"tu_ti033\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te028\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te021\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te022\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te023\" style=\"display: none;\"></div>\r\n\
<div id=\"tu_te024\" style=\"display: none;\"></div>\r\n\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te033\"><font color=\"#000\">Stan OFF</font></div></b></a><br><font class=\"b\">Termostat</font></div></div>\r\n\
</div></div>\r\n\
</div>\r\n\
</body>\r\n\
<script type=\"text/javascript\">\r\n\
window.onload = function(){\r\n\
Laduj();\r\n\
pageScroll();\r\n\
 };\r\n\
var active_id005=\"02p005\";\r\n\
var active_id006=\"09p006\";\r\n\
var active_id007=\"03p007\";\r\n\
var active_id008=\"04p008\";\r\n\
var active_id009=\"04p009\";\r\n\
var active_id010=\"06p010\";\r\n\
var active_id028=\"00p028\";\r\n\
var active_id021=\"00p021\";\r\n\
var active_id022=\"01p022\";\r\n\
var active_id023=\"01p023\";\r\n\
var active_id024=\"10p024\";\r\n\
var active_id033=\"11p033\";\r\n\
function Laduj()\r\n\
{\r\n\
var blok2 = document.getElementById(active_id005);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id006);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id007);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id008);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id009);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id010);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id028);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id021);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id022);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id023);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id024);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id033);\r\n\
blok2.style.display = \"block\";\r\n\
\r\n\
document.forms['wybor005'].n_GPIO_val_005.onchange = Zmien005;\r\n\
document.forms['wybor006'].n_GPIO_val_006.onchange = Zmien006;\r\n\
document.forms['wybor007'].n_GPIO_val_007.onchange = Zmien007;\r\n\
document.forms['wybor008'].n_GPIO_val_008.onchange = Zmien008;\r\n\
document.forms['wybor009'].n_GPIO_val_009.onchange = Zmien009;\r\n\
document.forms['wybor010'].n_GPIO_val_010.onchange = Zmien010;\r\n\
document.forms['wybor028'].n_GPIO_val_028.onchange = Zmien028;\r\n\
document.forms['wybor021'].n_GPIO_val_021.onchange = Zmien021;\r\n\
document.forms['wybor022'].n_GPIO_val_022.onchange = Zmien022;\r\n\
document.forms['wybor023'].n_GPIO_val_023.onchange = Zmien023;\r\n\
document.forms['wybor024'].n_GPIO_val_024.onchange = Zmien024;\r\n\
document.forms['wybor033'].n_GPIO_val_033.onchange = Zmien033;\r\n\
}\r\n\
 function Zmien005()\r\n\
 {\r\n\
 if (active_id005 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id005);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id005 = this.value;\r\n\
 }\r\n\
 function Zmien006()\r\n\
 {\r\n\
 if (active_id006 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id006);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id006 = this.value;\r\n\
 }\r\n\
 function Zmien007()\r\n\
 {\r\n\
 if (active_id007 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id007);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id007 = this.value;\r\n\
 }\r\n\
 function Zmien008()\r\n\
 {\r\n\
 if (active_id008 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id008);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id008 = this.value;\r\n\
 }\r\n\
 function Zmien009()\r\n\
 {\r\n\
 if (active_id009 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id009);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id009 = this.value;\r\n\
 }\r\n\
 function Zmien010()\r\n\
 {\r\n\
 if (active_id010 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id010);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id010 = this.value;\r\n\
 }\r\n\
 function Zmien028()\r\n\
 {\r\n\
 if (active_id028 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id028);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id028 = this.value;\r\n\
 }\r\n\
 function Zmien021()\r\n\
 {\r\n\
 if (active_id021 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id021);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id021 = this.value;\r\n\
 }\r\n\
 function Zmien022()\r\n\
 {\r\n\
 if (active_id022 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id022);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id022 = this.value;\r\n\
 }\r\n\
 function Zmien023()\r\n\
 {\r\n\
 if (active_id023 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id023);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id023 = this.value;\r\n\
 }\r\n\
 function Zmien024()\r\n\
 {\r\n\
 if (active_id024 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id024);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id024 = this.value;\r\n\
 }\r\n\
 function Zmien033()\r\n\
 {\r\n\
 if (active_id033 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id033);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id033 = this.value;\r\n\
 }\r\n\
function zaladuj(){var f=document.createElement('form');f.method='post';f.action='/lpc.cgi/A117';function add(n,v){var i=document.createElement('input');i.type='hidden';i.name=n;i.value=v;f.appendChild(i);}add('h7_page','/set');add('h7_form','1');var a=document.querySelectorAll('select[data-http-mode]');for(var j=0;j<a.length;j++)add(a[j].name,a[j].value.slice(0,2));add('n_109_zapisz','');document.body.appendChild(f);f.submit();}\r\n\
  </script>\r\n\
 \r\n\
 <script>\r\n\
 var nr_pwm; var val_pwm; var i,j,wst;  var txt=[]; \r\n\
 wst=0; \r\n\
 var now=0;void setInterval(function(){now++;if(now==2){wst=0;}},50);\r\n\
 var pwmBusy=[],pwmPending=[];function loadXMLDoc_PWM()\r\n\
 {var pwmChannel=nr_pwm,pwmValue=val_pwm;if(pwmBusy[pwmChannel]){pwmPending[pwmChannel]=pwmValue;return;}pwmBusy[pwmChannel]=true;var xmlhttp;       \r\n\
 if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
 else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n\
 xmlhttp.onreadystatechange=function(){if(xmlhttp.readyState==4){pwmBusy[pwmChannel]=false;if(pwmPending[pwmChannel]!==undefined){var latest=pwmPending[pwmChannel];delete pwmPending[pwmChannel];nr_pwm=pwmChannel;val_pwm=latest;loadXMLDoc_PWM();}}};xmlhttp.open(\"GET\",\"/lpc.cgi/n_pwm\"+pwmChannel+\"=\"+pwmValue+\"&n_115_zapisz=\", true);\r\n\
 xmlhttp.send();\r\n\
 }\r\n\
 function loadXMLDoc_PK()\r\n\
 { var xmlhttp; var controlPort=nr_pwm;       \r\n\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n\
   xmlhttp.onreadystatechange=function()\r\n\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200){\r\n\
	  txt= xmlhttp.responseText;\r\n\
	  if(txt[0]=='<'){\r\n\
if(controlPort==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\r\n\
if(controlPort==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\r\n\
}\r\n\
}\r\n\
}\r\n\
if(controlPort==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\r\n\
if(controlPort==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\r\n\
if(controlPort==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\r\n\
if(controlPort==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\r\n\
if(controlPort==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\r\n\
if(controlPort==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\r\n\
xmlhttp.send();\r\n\
}\r\n\
function xpk028(){controlPort=28;loadXMLDoc_PK();}\r\n\
function xpk021(){nr_pwm=21;loadXMLDoc_PK();}\r\n\
function xpk022(){nr_pwm=22;loadXMLDoc_PK();}\r\n\
function xpk023(){nr_pwm=23;loadXMLDoc_PK();}\r\n\
function xpk024(){nr_pwm=24;loadXMLDoc_PK();}\r\n\
function xpk033(){nr_pwm=33;loadXMLDoc_PK();}\r\n\
function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){nr_pwm=1;val_pwm=val;loadXMLDoc_PWM();}\r\n\
function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){nr_pwm=2;val_pwm=val;loadXMLDoc_PWM();}\r\n\
function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){nr_pwm=3;val_pwm=val;loadXMLDoc_PWM();}\r\n\
var myVar = setInterval(loadXMLDoc, 5000       );\r\n\
 var txt=[]; \r\n\
 var txt1=[]; \r\n\
 var txt2=[]; \r\n\
 var txt3=[]; \r\n\
 var txt4=[]; \r\n\
 var txt5=[]; \r\n\
 var ccol=[]; \r\n\
 function loadXMLDoc()\r\n\
 {if(wst==0){wst=1;       now=0;\r\n\
   var xmlhttp;\r\n\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n\
   xmlhttp.onreadystatechange=function()\r\n\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200)\r\n\
     {  \r\n\
		 txt= xmlhttp.responseText;\r\n\
		 txt3=txt[0]+txt[1]+txt[2]+txt[3]+txt[4]+txt[5]+txt[6]+txt[7]; document.getElementById(\"tu_zgr\").innerHTML=txt3;\r\n\
		 txt3=\"GSM: \"+txt[23]+txt[24]; document.getElementById(\"tu_gsm\").innerHTML=txt3;\r\n\
if((txt[25+0]=='0')||(txt[25+0]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk028\").innerHTML=txt3; document.getElementById(\"tu_ti028\").innerHTML=txt3; document.getElementById(\"tu_te028\").innerHTML=txt3;   if((txt[25+0]=='2')||(txt[25+0]=='3')) document.getElementById(\"tu_lora028wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora028wy\").innerHTML=\" \";\r\n\
if((txt[25+10]=='0')||(txt[25+10]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk021\").innerHTML=txt3; document.getElementById(\"tu_ti021\").innerHTML=txt3; document.getElementById(\"tu_te021\").innerHTML=txt3;   if((txt[25+10]=='2')||(txt[25+10]=='3')) document.getElementById(\"tu_lora021wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora021wy\").innerHTML=\" \";\r\n\
if((txt[25+11]=='0')||(txt[25+11]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk022\").innerHTML=txt3; document.getElementById(\"tu_ti022\").innerHTML=txt3; document.getElementById(\"tu_te022\").innerHTML=txt3;   if((txt[25+11]=='2')||(txt[25+11]=='3')) document.getElementById(\"tu_lora022wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora022wy\").innerHTML=\" \";\r\n\
if((txt[25+12]=='0')||(txt[25+12]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk023\").innerHTML=txt3; document.getElementById(\"tu_ti023\").innerHTML=txt3; document.getElementById(\"tu_te023\").innerHTML=txt3;   if((txt[25+12]=='2')||(txt[25+12]=='3')) document.getElementById(\"tu_lora023wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora023wy\").innerHTML=\" \";\r\n\
if((txt[25+6]=='0')||(txt[25+6]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk024\").innerHTML=txt3; document.getElementById(\"tu_ti024\").innerHTML=txt3; document.getElementById(\"tu_te024\").innerHTML=txt3;   if((txt[25+6]=='2')||(txt[25+6]=='3')) document.getElementById(\"tu_lora024wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora024wy\").innerHTML=\" \";\r\n\
if((txt[25+5]=='0')||(txt[25+5]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk033\").innerHTML=txt3; document.getElementById(\"tu_ti033\").innerHTML=txt3; document.getElementById(\"tu_te033\").innerHTML=txt3;   if((txt[25+5]=='2')||(txt[25+5]=='3')) document.getElementById(\"tu_lora033wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora033wy\").innerHTML=\" \";\r\n\
\r\n\
if((txt[25+13+1+4]=='0')||(txt[25+13+1+4]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we005\").innerHTML=txt3; if((txt[25+13+1+4]=='2')||(txt[25+13+1+4]=='3')) document.getElementById(\"tu_lora005we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+5]=='0')||(txt[25+13+1+5]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we006\").innerHTML=txt3; if((txt[25+13+1+5]=='2')||(txt[25+13+1+5]=='3')) document.getElementById(\"tu_lora006we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+6]=='0')||(txt[25+13+1+6]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we007\").innerHTML=txt3; if((txt[25+13+1+6]=='2')||(txt[25+13+1+6]=='3')) document.getElementById(\"tu_lora007we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+7]=='0')||(txt[25+13+1+7]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we008\").innerHTML=txt3; if((txt[25+13+1+7]=='2')||(txt[25+13+1+7]=='3')) document.getElementById(\"tu_lora008we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+8]=='0')||(txt[25+13+1+8]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we009\").innerHTML=txt3; if((txt[25+13+1+8]=='2')||(txt[25+13+1+8]=='3')) document.getElementById(\"tu_lora009we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+9]=='0')||(txt[25+13+1+9]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we010\").innerHTML=txt3; if((txt[25+13+1+9]=='2')||(txt[25+13+1+9]=='3')) document.getElementById(\"tu_lora010we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010we\").innerHTML=\" \";\r\n\
\r\n\
if((txt[25+13+1+18+1+4]=='5')||(txt[25+13+1+18+1+4]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+4]=='0')||(txt[25+13+1+18+1+4]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol005\").innerHTML=txt3; if((txt[25+13+1+18+1+4]=='2')||(txt[25+13+1+18+1+4]=='3')) document.getElementById(\"tu_lora005eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+5]=='5')||(txt[25+13+1+18+1+5]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+5]=='0')||(txt[25+13+1+18+1+5]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol006\").innerHTML=txt3; if((txt[25+13+1+18+1+5]=='2')||(txt[25+13+1+18+1+5]=='3')) document.getElementById(\"tu_lora006eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+6]=='5')||(txt[25+13+1+18+1+6]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+6]=='0')||(txt[25+13+1+18+1+6]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol007\").innerHTML=txt3; if((txt[25+13+1+18+1+6]=='2')||(txt[25+13+1+18+1+6]=='3')) document.getElementById(\"tu_lora007eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+7]=='5')||(txt[25+13+1+18+1+7]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+7]=='0')||(txt[25+13+1+18+1+7]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol008\").innerHTML=txt3; if((txt[25+13+1+18+1+7]=='2')||(txt[25+13+1+18+1+7]=='3')) document.getElementById(\"tu_lora008eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+8]=='5')||(txt[25+13+1+18+1+8]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+8]=='0')||(txt[25+13+1+18+1+8]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol009\").innerHTML=txt3; if((txt[25+13+1+18+1+8]=='2')||(txt[25+13+1+18+1+8]=='3')) document.getElementById(\"tu_lora009eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+9]=='5')||(txt[25+13+1+18+1+9]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+9]=='0')||(txt[25+13+1+18+1+9]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol010\").innerHTML=txt3; if((txt[25+13+1+18+1+9]=='2')||(txt[25+13+1+18+1+9]=='3')) document.getElementById(\"tu_lora010eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010eol\").innerHTML=\" \";\r\n\
\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+7+i];txt4[5]=txt[25+13+1+18+1+14+1+48+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+48+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+7+i];txt4[5]=txt[25+13+1+18+1+14+1+60+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+60+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+7+i];txt4[5]=txt[25+13+1+18+1+14+1+72+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+72+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+7+i];txt4[5]=txt[25+13+1+18+1+14+1+84+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+84+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+7+i];txt4[5]=txt[25+13+1+18+1+14+1+96+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+96+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+7+i];txt4[5]=txt[25+13+1+18+1+14+1+108+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+108+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
 \r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+40+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt005\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+50+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt006\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+60+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt007\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+70+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt008\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+80+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt009\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+90+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt010\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
\r\n\
 }\r\n\
 }\r\n\
 xmlhttp.open(\"GET\",\"/TME.txt\", true);\r\n\
 xmlhttp.send();\r\n\
 }}\r\n\
 </script> \r\n\
    <script>\r\n\
  var tyt=[];   tyt[0]=\"<li><a href='/nowa'>Panel graficzny LPC</a></li><li><a href='main'>Panel odczytowy</a></li><li><a href='3main'>Konfiguracja portow</a></li><li><a href='lan'>Ustawienia sieci LAN</a></li><li><a href='wifi'>Ustawienia sieci WiFi</a></li><li><a href='gsm'>Ustawienia sieci GSM</a></li><li><a href='czas'>Ustawienia czasu</a></li><li><a href='maskisms'>Numery telefonow</a></li><li><a href='maskimail'>Skrzynki e-mail</a></li><li><a href='/nrf'>Ustawienia LoRa</a></li><li><a href='logic'>Operacje logiczne</a></li><li><a href='rej'>Rejetrator</a></li><li><a href='Rot'>Rotacja</a></li><li><a href='/wykres1'>Wykres pomiarow</a></li><li><a href='/wykres2'>Wykres licznikow</a></li>\";\r\n\
  document.getElementById(\"tu_menu\").innerHTML=tyt;\r\n\
  </script>\r\n\
\r\n\
  \r\n\
</html>";



/* Settings page templates. Each function returns the next free byte.
 * Text values in measurement templates are HTML, like the original panel.
 */
static char* HTTP_TEMPLATE_2_Append(char* buff, const char* text)
{
	unsigned int len=mini_strlen(text);
	memcpy(buff,text,len+1);
	return buff+len;
}

static char* HTTP_TEMPLATE_2_Attribute(char* buff, const char* text)
{
	if(!text) return buff;
	while(*text){
		switch(*text){
		case '&': buff=HTTP_TEMPLATE_2_Append(buff,"&amp;"); break;
		case '"': buff=HTTP_TEMPLATE_2_Append(buff,"&quot;"); break;
		case '<': buff=HTTP_TEMPLATE_2_Append(buff,"&lt;"); break;
		case '>': buff=HTTP_TEMPLATE_2_Append(buff,"&gt;"); break;
		default: *buff++=*text; *buff=0; break;
		}
		text++;
	}
	return buff;
}

char* HTTP_TEMPLATE_2_NamesBegin(char* buff)
{
	buff=HTTP_TEMPLATE_2_Append(buff,"<style>.port-column{font-size:20px;line-height:normal;}.port-column>table{height:46px;width:100%;margin:0;border-spacing:2px;}.port-column>table:first-child{height:60px;}.port-column td{vertical-align:middle;}.port-column form{margin:0;}.port-readings>div[id]{height:46px;}.port-readings>div[id]>table{height:46px;margin:0;border-spacing:2px;}.port-readings table,.port-readings tbody{display:block;}.port-readings tr{display:flex;align-items:center;gap:10px;height:46px;box-sizing:border-box;padding-left:8px;}.port-readings td{display:flex;align-items:center;flex:0 0 auto;width:auto;padding:0;text-align:left;}.port-readings td:empty{display:none;}.port-readings>div[id] td:nth-child(2),.port-readings>table:first-child td:first-child{flex-basis:48px;width:48px;text-align:center;justify-content:center;}.port-readings>table:first-child tr{height:60px;}.port-readings>table:first-child td:nth-child(2){flex-basis:124px;justify-content:center;text-align:center;}.port-column>table.port-actions tr{display:flex;align-items:center;justify-content:center;height:46px;}.port-column>table.port-actions td{padding:0;}.port-column>table.port-actions td:empty{display:none;}.port-readings>div.port-unassigned[id] td:nth-child(2){flex-basis:auto;width:auto;margin-left:58px;justify-content:flex-start;white-space:nowrap;}.port-readings .shiny-button{box-sizing:border-box;min-width:34px;}.port-readings .m,.port-readings .c{box-sizing:border-box;min-width:124px;}.port-readings .v{height:auto;width:42px;line-height:normal;}.port-readings input[type=range]{width:110px;}</style><form method=\"post\" action=\"/lpc.cgi/A117\" data-ajax=\"false\">\r\n \r\n <input type=\"hidden\" name=\"h7_page\" value=\"/set\"><input type=\"hidden\" name=\"h7_form\" value=\"0\"><div class=\"corner-radius4\" style=\"margin-left: 0px; width: 340px; height:   1670px;   \">\r\n <div class=\"corner-radius3 port-column port-names\" style=\"margin-left: 10px; width: 310px; height:  1640px;   \">\r\n <table><tr><td width=\"30px\"></td><td><font class=\"css-font-style\" style=\"font-family: Courier\"><b>Nazwa Portu Nr:</b></font></td></tr></table>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_2_NamesEnd(char* buff)
{
	buff=HTTP_TEMPLATE_2_Append(buff,"<table class=\"port-actions\"><tr><td><button type=\"submit\" name=\"n_109_zapisz\" class=\"css_button\" style=\"padding: 2px 17px;\" >Zapisz Nazwy</button></td></tr></table>\r\n</div></div>\r\n</form>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_2_ModesBegin(char* buff)
{
	buff=HTTP_TEMPLATE_2_Append(buff,"<div class=\"corner-radius4\" style=\"margin-left: 0px; width: 340px; height:   1670px;   \">\r\n<div class=\"corner-radius3 port-column port-modes\" style=\"margin-left: 10px; width: 310px; height:  1640px;   \">\r\n<table><tr><td><font class=\"css-font-style\" style=\"font-family: Courier\"><b>Tryb Pracy Portu nr:</b></font><br></td></tr></table>\r\n\r\n");
	return buff;
}

char* HTTP_TEMPLATE_2_ModesEnd(char* buff)
{
	buff=HTTP_TEMPLATE_2_Append(buff," \r\n<table class=\"port-actions\"><tr><td><A HREF=\"javascript:zaladuj()\" class=\"css_button\" style=\"padding: 2px 17px;\">Zapisz Tryb</a></td></tr></table>\r\n</div></div>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_2_PanelBegin(char* buff)
{
	buff=HTTP_TEMPLATE_2_Append(buff,"<div class=\"corner-radius4\" style=\"margin-left: 0px; width: 370px; height:   1670px;   \">\r\n<div class=\"corner-radius3 port-column port-readings\" style=\"margin-left: 10px; width: 340px; height:  1640px;   \">\r\n<table><tr><td><font size=\"4\" color=\"73B10E\"><b>Ustaw</b></font></td><td><font size=\"4\" color=\"f0f0f0\"><b>Odczyty</b></font></td></tr></table>\r\n\r\n");
	return buff;
}

char* HTTP_TEMPLATE_2_PanelEnd(char* buff)
{
	buff=HTTP_TEMPLATE_2_Append(buff,"    </div></div>\r\n</div>\r\n</body>\r\n");
	return buff;
}

char* HTTP_TEMPLATE_2_Name(char* buff, u8 nr, const char* name)
{
	if(nr<1 || nr>33) return buff;
	buff += mini_snprintf(buff,250,"<table><tr><td><font class=\"css_list\">%d.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa%d_001\" size=\"30\" maxlength=\"30\" value=\"",nr,nr);
	buff=HTTP_TEMPLATE_2_Attribute(buff,name);
	buff=HTTP_TEMPLATE_2_Append(buff,"\"></td></tr></table>\r\n");
	return buff;
}

unsigned HTTP_SettingsPortMode(unsigned port)
{
	if(port<1 || port>33) return HTTP_MODE_DISABLED;
	uint16_t instance=0;
	const HTTP_SettingPage* page=HTTP_SettingsCatalogFind("/set",&instance);
	char field[32]; mini_snprintf(field,sizeof(field),"n_GPIO_val_%03u",port);
	const char* value=HTTP_SettingsGetByName(page,instance,field);
	return value?(unsigned)atoi(value):HTTP_MODE_DISABLED;
}

const char* HTTP_SettingsPortName(unsigned port, const char* fallback)
{
	if(port<1 || port>33) return fallback;
	uint16_t instance=0;
	const HTTP_SettingPage* page=HTTP_SettingsCatalogFind("/set",&instance);
	char field[40]; mini_snprintf(field,sizeof(field),"n_s_Port_nazwa%u_001",port);
	const char* saved=HTTP_SettingsGetSaved(page,instance,field);
	if(saved) return saved;
	if(fallback) return fallback;
	return HTTP_SettingsGetByName(page,instance,field);
}

char* HTTP_TEMPLATE_2_Mode(char* buff, u8 nr, u8 mode)
{
	if(nr<=20) return HTTP_TEMPLATE_ModeInput2(buff,nr,mode);
	return HTTP_TEMPLATE_ModeOutput2(buff,nr,mode);
}

char* HTTP_TEMPLATE_2_AdcTempRhu(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm)
{
	(void)route; (void)value1; (void)value2; (void)pwm;
	if(nr<1 || nr>33) return buff;
	switch(type){
	case HTTP_MODE_TEMP:
		if(!value1) value1="<font color='#000'>   --.- °C</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"02p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp%03d' class=\"shiny-button\">%d</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp%03da\">%s</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora%03da\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1,nr);
		break;
	case HTTP_MODE_TEMP_RHU:
		if(!value1) value1=" --.-";
		if(!value2) value2=" -- ";
		buff += mini_snprintf(buff,2000,"<div id=\"09p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp%03d' class=\"shiny-button\">%d</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp%03db\">%s</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp%03dc\">%s</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora%03db\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n",nr,route,nr,nr,value1,nr,value2,nr);
		break;
	case HTTP_MODE_PT1000:
		if(!value1) value1=" --- °C";
		buff += mini_snprintf(buff,2000,"<div id=\"03p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp%03d' class=\"shiny-button\">%d</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt%03d\">%s</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1);
		break;
	case HTTP_MODE_ADC:
		if(!value1) value1="     7.5   V";
		buff += mini_snprintf(buff,2000,"<div id=\"05p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='nap%03d' class=\"shiny-button\">%d</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_adc%03d\">%s</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1);
		break;
	default: break;
	}
	return buff;
}

char* HTTP_TEMPLATE_2_In(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm)
{
	(void)route; (void)value1; (void)value2; (void)pwm;
	if(nr<1 || nr>33) return buff;
	switch(type){
	case HTTP_MODE_INPUT:
		if(!value1) value1="<font color='#000'>Stan OFF</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"04p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we%03d' class=\"shiny-button\">%d</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we%03d\">%s</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora%03dwe\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1,nr);
		break;
	case HTTP_MODE_EOL:
		if(!value1) value1="<font color='#000'>Stan OFF</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"13p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol%03d' class=\"shiny-button\">%d</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol%03d\">%s</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora%03deol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1,nr);
		break;
	default: break;
	}
	return buff;
}

char* HTTP_TEMPLATE_2_Impl(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm)
{
	(void)route; (void)value1; (void)value2; (void)pwm;
	if(nr<1 || nr>33) return buff;
	switch(type){
	case HTTP_MODE_COUNTER:
		if(!value1) value1="0.000 KWh";
		buff += mini_snprintf(buff,2000,"<div id=\"07p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='etr%03d' class=\"shiny-button\">%d</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_imp0_%03d\">%s</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1);
		break;
	default: break;
	}
	return buff;
}

char* HTTP_TEMPLATE_2_Out(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm)
{
	(void)route; (void)value1; (void)value2; (void)pwm;
	if(nr<1 || nr>33) return buff;
	const RelayState* state=RELAY_GetState(nr);
	if(type==HTTP_MODE_PWM && state && state->pwm_valid) pwm=state->pwm_percent;
	if(type==HTTP_MODE_RELAY || type==HTTP_MODE_IR){
		int display=RELAY_GetDisplayState(nr);
		if(display>=0) value1=display?"<font color='#000'>Stan ON</font>":"<font color='#000'>Stan OFF</font>";
	}
	switch(type){
	case HTTP_MODE_RELAY:
		if(!value1) value1="<font color=\"#000\">Stan OFF</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"00p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk%03d' class=\"shiny-button\">%d</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk%03d(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk%03d\">%s</div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora%03dwy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,nr,value1,nr);
		break;
	case HTTP_MODE_TIMER:
		if(!value1) value1="<font color=\"#000\">Stan OFF</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"10p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim%03d' class=\"shiny-button\">%d</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti%03d\">%s</div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1);
		break;
	case HTTP_MODE_THERMOSTAT:
		if(!value1) value1="<font color=\"#000\">Stan OFF</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"11p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term%03d' class=\"shiny-button\">%d</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te%03d\">%s</div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,value1);
		break;
	case HTTP_MODE_IR:
		if(!value1) value1="<font color=\"#000\">Stan OFF</font>";
		buff += mini_snprintf(buff,2000,"<div id=\"08p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk%03d' class=\"shiny-button\">%d</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk%03d(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk%03d\">%s</div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n",nr,route,nr,nr,nr,value1);
		break;
	case HTTP_MODE_PWM:
		buff += mini_snprintf(buff,2000,"<div id=\"01p%03d\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='3main' class=\"shiny-button\">%d</a></td><td width=\"12px\"></td><td width=\"42px\"><output  class=\"v\" id=\"volume%03d\">%d</output></td><td><input type=\"range\" name=\"n_pwm%d\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"%d\"   oninput=\"outputUpdate%03d(value)\" onchange=\"x%03d(this.value);\"></input></td></tr></table></div>\r\n",nr,nr,nr,pwm,nr-20,pwm,nr,nr);
		break;
	default: break;
	}
	return buff;
}

char* HTTP_TEMPLATE_2_Hidden(char* buff, u8 nr)
{
	static const u8 order[]={HTTP_MODE_RELAY,HTTP_MODE_TEMP,HTTP_MODE_TEMP_RHU,HTTP_MODE_TIMER,HTTP_MODE_THERMOSTAT,HTTP_MODE_IR,HTTP_MODE_INPUT,HTTP_MODE_EOL,HTTP_MODE_PWM,HTTP_MODE_ADC,HTTP_MODE_PT1000,HTTP_MODE_COUNTER};
	unsigned int i;
	if(nr<1 || nr>33) return buff;
	for(i=0;i<sizeof(order)/sizeof(order[0]);i++){
		u8 mode=order[i];
		int supported;
		if(nr<=14) supported=(mode==HTTP_MODE_TEMP || mode==HTTP_MODE_PT1000 || mode==HTTP_MODE_TEMP_RHU || mode==HTTP_MODE_INPUT || mode==HTTP_MODE_EOL);
		else if(nr<=16) supported=(mode==HTTP_MODE_ADC);
		else if(nr<=20) supported=(mode==HTTP_MODE_TEMP || mode==HTTP_MODE_TEMP_RHU || mode==HTTP_MODE_INPUT || mode==HTTP_MODE_COUNTER);
		else supported=(mode==HTTP_MODE_RELAY || mode==HTTP_MODE_TIMER || mode==HTTP_MODE_THERMOSTAT || (nr<=27 && mode==HTTP_MODE_IR) || (nr<=23 && mode==HTTP_MODE_PWM));
		if(!supported) buff += mini_snprintf(buff,100,"<div id=\"%02dp%03d\" style=\"display: none;\"></div>\r\n",mode,nr);
	}
	buff += mini_snprintf(buff,300,"<div class=\"port-unassigned\" id=\"06p%03d\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\">---&nbsp; Wybierz funkcję&nbsp; ---</font></td></tr></table></div>\r\n",nr);
	return buff;
}

const char HttpStructMainReadPanel2[]=
"<script type=\"text/javascript\">\r\n"
"window.onload = function(){\r\n"
"Laduj();\r\n"
"pageScroll();\r\n"
" };\r\n"
"var active_id001=document.forms['wybor001'].n_GPIO_val_001.value;\r\n"
"var active_id002=document.forms['wybor002'].n_GPIO_val_002.value;\r\n"
"var active_id003=document.forms['wybor003'].n_GPIO_val_003.value;\r\n"
"var active_id004=document.forms['wybor004'].n_GPIO_val_004.value;\r\n"
"var active_id005=document.forms['wybor005'].n_GPIO_val_005.value;\r\n"
"var active_id006=document.forms['wybor006'].n_GPIO_val_006.value;\r\n"
"var active_id007=document.forms['wybor007'].n_GPIO_val_007.value;\r\n"
"var active_id008=document.forms['wybor008'].n_GPIO_val_008.value;\r\n"
"var active_id009=document.forms['wybor009'].n_GPIO_val_009.value;\r\n"
"var active_id010=document.forms['wybor010'].n_GPIO_val_010.value;\r\n"
"var active_id011=document.forms['wybor011'].n_GPIO_val_011.value;\r\n"
"var active_id012=document.forms['wybor012'].n_GPIO_val_012.value;\r\n"
"var active_id013=document.forms['wybor013'].n_GPIO_val_013.value;\r\n"
"var active_id014=document.forms['wybor014'].n_GPIO_val_014.value;\r\n"
"var active_id015=document.forms['wybor015'].n_GPIO_val_015.value;\r\n"
"var active_id016=document.forms['wybor016'].n_GPIO_val_016.value;\r\n"
"var active_id017=document.forms['wybor017'].n_GPIO_val_017.value;\r\n"
"var active_id018=document.forms['wybor018'].n_GPIO_val_018.value;\r\n"
"var active_id019=document.forms['wybor019'].n_GPIO_val_019.value;\r\n"
"var active_id020=document.forms['wybor020'].n_GPIO_val_020.value;\r\n"
"var active_id021=document.forms['wybor021'].n_GPIO_val_021.value;\r\n"
"var active_id022=document.forms['wybor022'].n_GPIO_val_022.value;\r\n"
"var active_id023=document.forms['wybor023'].n_GPIO_val_023.value;\r\n"
"var active_id024=document.forms['wybor024'].n_GPIO_val_024.value;\r\n"
"var active_id025=document.forms['wybor025'].n_GPIO_val_025.value;\r\n"
"var active_id026=document.forms['wybor026'].n_GPIO_val_026.value;\r\n"
"var active_id027=document.forms['wybor027'].n_GPIO_val_027.value;\r\n"
"var active_id028=document.forms['wybor028'].n_GPIO_val_028.value;\r\n"
"var active_id029=document.forms['wybor029'].n_GPIO_val_029.value;\r\n"
"var active_id030=document.forms['wybor030'].n_GPIO_val_030.value;\r\n"
"var active_id031=document.forms['wybor031'].n_GPIO_val_031.value;\r\n"
"var active_id032=document.forms['wybor032'].n_GPIO_val_032.value;\r\n"
"var active_id033=document.forms['wybor033'].n_GPIO_val_033.value;\r\n"
"function Laduj()\r\n"
"{\r\n"
"var blok2 = document.getElementById(active_id001);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id002);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id003);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id004);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id005);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id006);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id007);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id008);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id009);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id010);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id011);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id012);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id013);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id014);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id015);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id016);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id017);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id018);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id019);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id020);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id021);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id022);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id023);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id024);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id025);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id026);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id027);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id028);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id029);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id030);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id031);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id032);\r\n"
"blok2.style.display = \"block\";\r\n"
"var blok2 = document.getElementById(active_id033);\r\n"
"blok2.style.display = \"block\";\r\n"
"\r\n"
"document.forms['wybor001'].n_GPIO_val_001.onchange = Zmien001;\r\n"
"document.forms['wybor002'].n_GPIO_val_002.onchange = Zmien002;\r\n"
"document.forms['wybor003'].n_GPIO_val_003.onchange = Zmien003;\r\n"
"document.forms['wybor004'].n_GPIO_val_004.onchange = Zmien004;\r\n"
"document.forms['wybor005'].n_GPIO_val_005.onchange = Zmien005;\r\n"
"document.forms['wybor006'].n_GPIO_val_006.onchange = Zmien006;\r\n"
"document.forms['wybor007'].n_GPIO_val_007.onchange = Zmien007;\r\n"
"document.forms['wybor008'].n_GPIO_val_008.onchange = Zmien008;\r\n"
"document.forms['wybor009'].n_GPIO_val_009.onchange = Zmien009;\r\n"
"document.forms['wybor010'].n_GPIO_val_010.onchange = Zmien010;\r\n"
"document.forms['wybor011'].n_GPIO_val_011.onchange = Zmien011;\r\n"
"document.forms['wybor012'].n_GPIO_val_012.onchange = Zmien012;\r\n"
"document.forms['wybor013'].n_GPIO_val_013.onchange = Zmien013;\r\n"
"document.forms['wybor014'].n_GPIO_val_014.onchange = Zmien014;\r\n"
"document.forms['wybor015'].n_GPIO_val_015.onchange = Zmien015;\r\n"
"document.forms['wybor016'].n_GPIO_val_016.onchange = Zmien016;\r\n"
"document.forms['wybor017'].n_GPIO_val_017.onchange = Zmien017;\r\n"
"document.forms['wybor018'].n_GPIO_val_018.onchange = Zmien018;\r\n"
"document.forms['wybor019'].n_GPIO_val_019.onchange = Zmien019;\r\n"
"document.forms['wybor020'].n_GPIO_val_020.onchange = Zmien020;\r\n"
"document.forms['wybor021'].n_GPIO_val_021.onchange = Zmien021;\r\n"
"document.forms['wybor022'].n_GPIO_val_022.onchange = Zmien022;\r\n"
"document.forms['wybor023'].n_GPIO_val_023.onchange = Zmien023;\r\n"
"document.forms['wybor024'].n_GPIO_val_024.onchange = Zmien024;\r\n"
"document.forms['wybor025'].n_GPIO_val_025.onchange = Zmien025;\r\n"
"document.forms['wybor026'].n_GPIO_val_026.onchange = Zmien026;\r\n"
"document.forms['wybor027'].n_GPIO_val_027.onchange = Zmien027;\r\n"
"document.forms['wybor028'].n_GPIO_val_028.onchange = Zmien028;\r\n"
"document.forms['wybor029'].n_GPIO_val_029.onchange = Zmien029;\r\n"
"document.forms['wybor030'].n_GPIO_val_030.onchange = Zmien030;\r\n"
"document.forms['wybor031'].n_GPIO_val_031.onchange = Zmien031;\r\n"
"document.forms['wybor032'].n_GPIO_val_032.onchange = Zmien032;\r\n"
"document.forms['wybor033'].n_GPIO_val_033.onchange = Zmien033;\r\n"
"}\r\n"
" function Zmien001()\r\n"
" {\r\n"
" if (active_id001 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id001);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id001 = this.value;\r\n"
" }\r\n"
" function Zmien002()\r\n"
" {\r\n"
" if (active_id002 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id002);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id002 = this.value;\r\n"
" }\r\n"
" function Zmien003()\r\n"
" {\r\n"
" if (active_id003 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id003);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id003 = this.value;\r\n"
" }\r\n"
" function Zmien004()\r\n"
" {\r\n"
" if (active_id004 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id004);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id004 = this.value;\r\n"
" }\r\n"
" function Zmien005()\r\n"
" {\r\n"
" if (active_id005 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id005);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id005 = this.value;\r\n"
" }\r\n"
" function Zmien006()\r\n"
" {\r\n"
" if (active_id006 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id006);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id006 = this.value;\r\n"
" }\r\n"
" function Zmien007()\r\n"
" {\r\n"
" if (active_id007 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id007);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id007 = this.value;\r\n"
" }\r\n"
" function Zmien008()\r\n"
" {\r\n"
" if (active_id008 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id008);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id008 = this.value;\r\n"
" }\r\n"
" function Zmien009()\r\n"
" {\r\n"
" if (active_id009 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id009);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id009 = this.value;\r\n"
" }\r\n"
" function Zmien010()\r\n"
" {\r\n"
" if (active_id010 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id010);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id010 = this.value;\r\n"
" }\r\n"
" function Zmien011()\r\n"
" {\r\n"
" if (active_id011 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id011);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id011 = this.value;\r\n"
" }\r\n"
" function Zmien012()\r\n"
" {\r\n"
" if (active_id012 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id012);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id012 = this.value;\r\n"
" }\r\n"
" function Zmien013()\r\n"
" {\r\n"
" if (active_id013 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id013);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id013 = this.value;\r\n"
" }\r\n"
" function Zmien014()\r\n"
" {\r\n"
" if (active_id014 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id014);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id014 = this.value;\r\n"
" }\r\n"
" function Zmien015()\r\n"
" {\r\n"
" if (active_id015 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id015);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id015 = this.value;\r\n"
" }\r\n"
" function Zmien016()\r\n"
" {\r\n"
" if (active_id016 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id016);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id016 = this.value;\r\n"
" }\r\n"
" function Zmien017()\r\n"
" {\r\n"
" if (active_id017 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id017);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id017 = this.value;\r\n"
" }\r\n"
" function Zmien018()\r\n"
" {\r\n"
" if (active_id018 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id018);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id018 = this.value;\r\n"
" }\r\n"
" function Zmien019()\r\n"
" {\r\n"
" if (active_id019 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id019);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id019 = this.value;\r\n"
" }\r\n"
" function Zmien020()\r\n"
" {\r\n"
" if (active_id020 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id020);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id020 = this.value;\r\n"
" }\r\n"
" function Zmien021()\r\n"
" {\r\n"
" if (active_id021 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id021);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id021 = this.value;\r\n"
" }\r\n"
" function Zmien022()\r\n"
" {\r\n"
" if (active_id022 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id022);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id022 = this.value;\r\n"
" }\r\n"
" function Zmien023()\r\n"
" {\r\n"
" if (active_id023 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id023);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id023 = this.value;\r\n"
" }\r\n"
" function Zmien024()\r\n"
" {\r\n"
" if (active_id024 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id024);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id024 = this.value;\r\n"
" }\r\n"
" function Zmien025()\r\n"
" {\r\n"
" if (active_id025 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id025);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id025 = this.value;\r\n"
" }\r\n"
" function Zmien026()\r\n"
" {\r\n"
" if (active_id026 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id026);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id026 = this.value;\r\n"
" }\r\n"
" function Zmien027()\r\n"
" {\r\n"
" if (active_id027 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id027);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id027 = this.value;\r\n"
" }\r\n"
" function Zmien028()\r\n"
" {\r\n"
" if (active_id028 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id028);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id028 = this.value;\r\n"
" }\r\n"
" function Zmien029()\r\n"
" {\r\n"
" if (active_id029 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id029);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id029 = this.value;\r\n"
" }\r\n"
" function Zmien030()\r\n"
" {\r\n"
" if (active_id030 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id030);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id030 = this.value;\r\n"
" }\r\n"
" function Zmien031()\r\n"
" {\r\n"
" if (active_id031 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id031);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id031 = this.value;\r\n"
" }\r\n"
" function Zmien032()\r\n"
" {\r\n"
" if (active_id032 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id032);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id032 = this.value;\r\n"
" }\r\n"
" function Zmien033()\r\n"
" {\r\n"
" if (active_id033 != \"\")\r\n"
" {\r\n"
" var blok2 = document.getElementById(active_id033);\r\n"
" blok2.style.display = \"none\";\r\n"
" }\r\n"
" var blok = document.getElementById(this.value);\r\n"
" blok.style.display = \"block\";\r\n"
" active_id033 = this.value;\r\n"
" }\r\n"
"function zaladuj(){var f=document.createElement('form');f.method='post';f.action='/lpc.cgi/A117';function add(n,v){var i=document.createElement('input');i.type='hidden';i.name=n;i.value=v;f.appendChild(i);}add('h7_page','/set');add('h7_form','1');var a=document.querySelectorAll('select[data-http-mode]');for(var j=0;j<a.length;j++)add(a[j].name,a[j].value.slice(0,2));add('n_109_zapisz','');document.body.appendChild(f);f.submit();}\r\n"
"  </script>\r\n"
" \r\n"
" <script>\r\n"
" var nr_pwm; var val_pwm; var i,j,wst;  var txt=[]; \r\n"
" wst=0; \r\n"
" var now=0;void setInterval(function(){now++;if(now==2){wst=0;}},50);\r\n"
" var pwmBusy=[],pwmPending=[];function loadXMLDoc_PWM()\r\n"
" {var pwmChannel=nr_pwm,pwmValue=val_pwm;if(pwmBusy[pwmChannel]){pwmPending[pwmChannel]=pwmValue;return;}pwmBusy[pwmChannel]=true;var xmlhttp;       \r\n"
" if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n"
" else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n"
" xmlhttp.onreadystatechange=function(){if(xmlhttp.readyState==4){pwmBusy[pwmChannel]=false;if(pwmPending[pwmChannel]!==undefined){var latest=pwmPending[pwmChannel];delete pwmPending[pwmChannel];nr_pwm=pwmChannel;val_pwm=latest;loadXMLDoc_PWM();}}};xmlhttp.open(\"GET\",\"/lpc.cgi/n_pwm\"+pwmChannel+\"=\"+pwmValue+\"&n_115_zapisz=\", true);\r\n"
" xmlhttp.send();\r\n"
" }\r\n"
" function loadXMLDoc_PK()\r\n"
" { var xmlhttp; var controlPort=nr_pwm;       \r\n"
"   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n"
"   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n"
"   xmlhttp.onreadystatechange=function()\r\n"
"   { if (xmlhttp.readyState==4 && xmlhttp.status==200){\r\n"
"\t  txt= xmlhttp.responseText;\r\n"
"\t  if(txt[0]=='<'){\r\n"
"if(controlPort==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==25) document.getElementById(\"tu_pk025\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==26) document.getElementById(\"tu_pk026\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==27) document.getElementById(\"tu_pk027\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==29) document.getElementById(\"tu_pk029\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==30) document.getElementById(\"tu_pk030\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==31) document.getElementById(\"tu_pk031\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==32) document.getElementById(\"tu_pk032\").innerHTML=xmlhttp.responseText;\r\n"
"if(controlPort==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\r\n"
"}\r\n"
"}\r\n"
"}\r\n"
"if(controlPort==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\r\n"
"if(controlPort==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\r\n"
"if(controlPort==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\r\n"
"if(controlPort==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\r\n"
"if(controlPort==25) xmlhttp.open(\"GET\",\"/mobile/pk008\", true);\r\n"
"if(controlPort==26) xmlhttp.open(\"GET\",\"/mobile/pk009\", true);\r\n"
"if(controlPort==27) xmlhttp.open(\"GET\",\"/mobile/pk010\", true);\r\n"
"if(controlPort==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\r\n"
"if(controlPort==29) xmlhttp.open(\"GET\",\"/mobile/pk002\", true);\r\n"
"if(controlPort==30) xmlhttp.open(\"GET\",\"/mobile/pk003\", true);\r\n"
"if(controlPort==31) xmlhttp.open(\"GET\",\"/mobile/pk004\", true);\r\n"
"if(controlPort==32) xmlhttp.open(\"GET\",\"/mobile/pk005\", true);\r\n"
"if(controlPort==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\r\n"
"xmlhttp.send();\r\n"
"}\r\n"
"function xpk021(){nr_pwm=21;loadXMLDoc_PK();}\r\n"
"function xpk022(){nr_pwm=22;loadXMLDoc_PK();}\r\n"
"function xpk023(){nr_pwm=23;loadXMLDoc_PK();}\r\n"
"function xpk024(){nr_pwm=24;loadXMLDoc_PK();}\r\n"
"function xpk025(){nr_pwm=25;loadXMLDoc_PK();}\r\n"
"function xpk026(){nr_pwm=26;loadXMLDoc_PK();}\r\n"
"function xpk027(){nr_pwm=27;loadXMLDoc_PK();}\r\n"
"function xpk028(){nr_pwm=28;loadXMLDoc_PK();}\r\n"
"function xpk029(){nr_pwm=29;loadXMLDoc_PK();}\r\n"
"function xpk030(){nr_pwm=30;loadXMLDoc_PK();}\r\n"
"function xpk031(){nr_pwm=31;loadXMLDoc_PK();}\r\n"
"function xpk032(){nr_pwm=32;loadXMLDoc_PK();}\r\n"
"function xpk033(){nr_pwm=33;loadXMLDoc_PK();}\r\n"
"function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){nr_pwm=1;val_pwm=val;loadXMLDoc_PWM();}\r\n"
"function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){nr_pwm=2;val_pwm=val;loadXMLDoc_PWM();}\r\n"
"function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){nr_pwm=3;val_pwm=val;loadXMLDoc_PWM();}\r\n"
"var myVar = setInterval(loadXMLDoc, 5000      );\r\n"
" var txt=[]; \r\n"
" var txt1=[]; \r\n"
" var txt2=[]; \r\n"
" var txt3=[]; \r\n"
" var txt4=[]; \r\n"
" var txt5=[]; \r\n"
" var ccol=[]; \r\n"
" function loadXMLDoc()\r\n"
" {if(wst==0){wst=1;       now=0;\r\n"
"   var xmlhttp;\r\n"
"   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n"
"   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n"
"   xmlhttp.onreadystatechange=function()\r\n"
"   { if (xmlhttp.readyState==4 && xmlhttp.status==200)\r\n"
"     {  \r\n"
"\t\t txt= xmlhttp.responseText;\r\n"
"\t\t txt3=txt[0]+txt[1]+txt[2]+txt[3]+txt[4]+txt[5]+txt[6]+txt[7]; document.getElementById(\"tu_zgr\").innerHTML=txt3;\r\n"
"\t\t txt3=\"GSM: \"+txt[23]+txt[24]; document.getElementById(\"tu_gsm\").innerHTML=txt3;\r\n"
"if((txt[25+10]=='0')||(txt[25+10]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk021\").innerHTML=txt3; document.getElementById(\"tu_ti021\").innerHTML=txt3; document.getElementById(\"tu_te021\").innerHTML=txt3;   if((txt[25+10]=='2')||(txt[25+10]=='3')) document.getElementById(\"tu_lora021wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora021wy\").innerHTML=\" \";\r\n"
"if((txt[25+11]=='0')||(txt[25+11]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk022\").innerHTML=txt3; document.getElementById(\"tu_ti022\").innerHTML=txt3; document.getElementById(\"tu_te022\").innerHTML=txt3;   if((txt[25+11]=='2')||(txt[25+11]=='3')) document.getElementById(\"tu_lora022wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora022wy\").innerHTML=\" \";\r\n"
"if((txt[25+12]=='0')||(txt[25+12]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk023\").innerHTML=txt3; document.getElementById(\"tu_ti023\").innerHTML=txt3; document.getElementById(\"tu_te023\").innerHTML=txt3;   if((txt[25+12]=='2')||(txt[25+12]=='3')) document.getElementById(\"tu_lora023wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora023wy\").innerHTML=\" \";\r\n"
"if((txt[25+6]=='0')||(txt[25+6]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk024\").innerHTML=txt3; document.getElementById(\"tu_ti024\").innerHTML=txt3; document.getElementById(\"tu_te024\").innerHTML=txt3;   if((txt[25+6]=='2')||(txt[25+6]=='3')) document.getElementById(\"tu_lora024wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora024wy\").innerHTML=\" \";\r\n"
"if((txt[25+7]=='0')||(txt[25+7]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk025\").innerHTML=txt3; document.getElementById(\"tu_ti025\").innerHTML=txt3; document.getElementById(\"tu_te025\").innerHTML=txt3;   if((txt[25+7]=='2')||(txt[25+7]=='3')) document.getElementById(\"tu_lora025wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora025wy\").innerHTML=\" \";\r\n"
"if((txt[25+8]=='0')||(txt[25+8]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk026\").innerHTML=txt3; document.getElementById(\"tu_ti026\").innerHTML=txt3; document.getElementById(\"tu_te026\").innerHTML=txt3;   if((txt[25+8]=='2')||(txt[25+8]=='3')) document.getElementById(\"tu_lora026wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora026wy\").innerHTML=\" \";\r\n"
"if((txt[25+9]=='0')||(txt[25+9]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk027\").innerHTML=txt3; document.getElementById(\"tu_ti027\").innerHTML=txt3; document.getElementById(\"tu_te027\").innerHTML=txt3;   if((txt[25+9]=='2')||(txt[25+9]=='3')) document.getElementById(\"tu_lora027wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora027wy\").innerHTML=\" \";\r\n"
"if((txt[25+0]=='0')||(txt[25+0]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk028\").innerHTML=txt3; document.getElementById(\"tu_ti028\").innerHTML=txt3; document.getElementById(\"tu_te028\").innerHTML=txt3;   if((txt[25+0]=='2')||(txt[25+0]=='3')) document.getElementById(\"tu_lora028wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora028wy\").innerHTML=\" \";\r\n"
"if((txt[25+1]=='0')||(txt[25+1]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk029\").innerHTML=txt3; document.getElementById(\"tu_ti029\").innerHTML=txt3; document.getElementById(\"tu_te029\").innerHTML=txt3;   if((txt[25+1]=='2')||(txt[25+1]=='3')) document.getElementById(\"tu_lora029wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora029wy\").innerHTML=\" \";\r\n"
"if((txt[25+2]=='0')||(txt[25+2]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk030\").innerHTML=txt3; document.getElementById(\"tu_ti030\").innerHTML=txt3; document.getElementById(\"tu_te030\").innerHTML=txt3;   if((txt[25+2]=='2')||(txt[25+2]=='3')) document.getElementById(\"tu_lora030wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora030wy\").innerHTML=\" \";\r\n"
"if((txt[25+3]=='0')||(txt[25+3]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk031\").innerHTML=txt3; document.getElementById(\"tu_ti031\").innerHTML=txt3; document.getElementById(\"tu_te031\").innerHTML=txt3;   if((txt[25+3]=='2')||(txt[25+3]=='3')) document.getElementById(\"tu_lora031wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora031wy\").innerHTML=\" \";\r\n"
"if((txt[25+4]=='0')||(txt[25+4]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk032\").innerHTML=txt3; document.getElementById(\"tu_ti032\").innerHTML=txt3; document.getElementById(\"tu_te032\").innerHTML=txt3;   if((txt[25+4]=='2')||(txt[25+4]=='3')) document.getElementById(\"tu_lora032wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora032wy\").innerHTML=\" \";\r\n"
"if((txt[25+5]=='0')||(txt[25+5]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk033\").innerHTML=txt3; document.getElementById(\"tu_ti033\").innerHTML=txt3; document.getElementById(\"tu_te033\").innerHTML=txt3;   if((txt[25+5]=='2')||(txt[25+5]=='3')) document.getElementById(\"tu_lora033wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora033wy\").innerHTML=\" \";\r\n"
"\r\n"
"if((txt[25+13+1+0]=='0')||(txt[25+13+1+0]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we001\").innerHTML=txt3; if((txt[25+13+1+0]=='2')||(txt[25+13+1+0]=='3')) document.getElementById(\"tu_lora001we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+1]=='0')||(txt[25+13+1+1]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we002\").innerHTML=txt3; if((txt[25+13+1+1]=='2')||(txt[25+13+1+1]=='3')) document.getElementById(\"tu_lora002we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+2]=='0')||(txt[25+13+1+2]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we003\").innerHTML=txt3; if((txt[25+13+1+2]=='2')||(txt[25+13+1+2]=='3')) document.getElementById(\"tu_lora003we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+3]=='0')||(txt[25+13+1+3]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we004\").innerHTML=txt3; if((txt[25+13+1+3]=='2')||(txt[25+13+1+3]=='3')) document.getElementById(\"tu_lora004we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+4]=='0')||(txt[25+13+1+4]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we005\").innerHTML=txt3; if((txt[25+13+1+4]=='2')||(txt[25+13+1+4]=='3')) document.getElementById(\"tu_lora005we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+5]=='0')||(txt[25+13+1+5]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we006\").innerHTML=txt3; if((txt[25+13+1+5]=='2')||(txt[25+13+1+5]=='3')) document.getElementById(\"tu_lora006we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+6]=='0')||(txt[25+13+1+6]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we007\").innerHTML=txt3; if((txt[25+13+1+6]=='2')||(txt[25+13+1+6]=='3')) document.getElementById(\"tu_lora007we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+7]=='0')||(txt[25+13+1+7]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we008\").innerHTML=txt3; if((txt[25+13+1+7]=='2')||(txt[25+13+1+7]=='3')) document.getElementById(\"tu_lora008we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+8]=='0')||(txt[25+13+1+8]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we009\").innerHTML=txt3; if((txt[25+13+1+8]=='2')||(txt[25+13+1+8]=='3')) document.getElementById(\"tu_lora009we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+9]=='0')||(txt[25+13+1+9]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we010\").innerHTML=txt3; if((txt[25+13+1+9]=='2')||(txt[25+13+1+9]=='3')) document.getElementById(\"tu_lora010we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+10]=='0')||(txt[25+13+1+10]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we011\").innerHTML=txt3; if((txt[25+13+1+10]=='2')||(txt[25+13+1+10]=='3')) document.getElementById(\"tu_lora011we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+11]=='0')||(txt[25+13+1+11]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we012\").innerHTML=txt3; if((txt[25+13+1+11]=='2')||(txt[25+13+1+11]=='3')) document.getElementById(\"tu_lora012we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+12]=='0')||(txt[25+13+1+12]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we013\").innerHTML=txt3; if((txt[25+13+1+12]=='2')||(txt[25+13+1+12]=='3')) document.getElementById(\"tu_lora013we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+13]=='0')||(txt[25+13+1+13]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we014\").innerHTML=txt3; if((txt[25+13+1+13]=='2')||(txt[25+13+1+13]=='3')) document.getElementById(\"tu_lora014we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+14]=='0')||(txt[25+13+1+14]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we017\").innerHTML=txt3; if((txt[25+13+1+14]=='2')||(txt[25+13+1+14]=='3')) document.getElementById(\"tu_lora017we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+15]=='0')||(txt[25+13+1+15]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we018\").innerHTML=txt3; if((txt[25+13+1+15]=='2')||(txt[25+13+1+15]=='3')) document.getElementById(\"tu_lora018we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+16]=='0')||(txt[25+13+1+16]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we019\").innerHTML=txt3; if((txt[25+13+1+16]=='2')||(txt[25+13+1+16]=='3')) document.getElementById(\"tu_lora019we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019we\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+17]=='0')||(txt[25+13+1+17]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we020\").innerHTML=txt3; if((txt[25+13+1+17]=='2')||(txt[25+13+1+17]=='3')) document.getElementById(\"tu_lora020we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020we\").innerHTML=\" \";\r\n"
"\r\n"
"if((txt[25+13+1+18+1+0]=='5')||(txt[25+13+1+18+1+0]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+0]=='0')||(txt[25+13+1+18+1+0]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol001\").innerHTML=txt3; if((txt[25+13+1+18+1+0]=='2')||(txt[25+13+1+18+1+0]=='3')) document.getElementById(\"tu_lora001eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+1]=='5')||(txt[25+13+1+18+1+1]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+1]=='0')||(txt[25+13+1+18+1+1]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol002\").innerHTML=txt3; if((txt[25+13+1+18+1+1]=='2')||(txt[25+13+1+18+1+1]=='3')) document.getElementById(\"tu_lora002eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+2]=='5')||(txt[25+13+1+18+1+2]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+2]=='0')||(txt[25+13+1+18+1+2]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol003\").innerHTML=txt3; if((txt[25+13+1+18+1+2]=='2')||(txt[25+13+1+18+1+2]=='3')) document.getElementById(\"tu_lora003eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+3]=='5')||(txt[25+13+1+18+1+3]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+3]=='0')||(txt[25+13+1+18+1+3]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol004\").innerHTML=txt3; if((txt[25+13+1+18+1+3]=='2')||(txt[25+13+1+18+1+3]=='3')) document.getElementById(\"tu_lora004eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+4]=='5')||(txt[25+13+1+18+1+4]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+4]=='0')||(txt[25+13+1+18+1+4]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol005\").innerHTML=txt3; if((txt[25+13+1+18+1+4]=='2')||(txt[25+13+1+18+1+4]=='3')) document.getElementById(\"tu_lora005eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+5]=='5')||(txt[25+13+1+18+1+5]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+5]=='0')||(txt[25+13+1+18+1+5]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol006\").innerHTML=txt3; if((txt[25+13+1+18+1+5]=='2')||(txt[25+13+1+18+1+5]=='3')) document.getElementById(\"tu_lora006eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+6]=='5')||(txt[25+13+1+18+1+6]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+6]=='0')||(txt[25+13+1+18+1+6]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol007\").innerHTML=txt3; if((txt[25+13+1+18+1+6]=='2')||(txt[25+13+1+18+1+6]=='3')) document.getElementById(\"tu_lora007eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+7]=='5')||(txt[25+13+1+18+1+7]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+7]=='0')||(txt[25+13+1+18+1+7]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol008\").innerHTML=txt3; if((txt[25+13+1+18+1+7]=='2')||(txt[25+13+1+18+1+7]=='3')) document.getElementById(\"tu_lora008eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+8]=='5')||(txt[25+13+1+18+1+8]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+8]=='0')||(txt[25+13+1+18+1+8]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol009\").innerHTML=txt3; if((txt[25+13+1+18+1+8]=='2')||(txt[25+13+1+18+1+8]=='3')) document.getElementById(\"tu_lora009eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+9]=='5')||(txt[25+13+1+18+1+9]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+9]=='0')||(txt[25+13+1+18+1+9]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol010\").innerHTML=txt3; if((txt[25+13+1+18+1+9]=='2')||(txt[25+13+1+18+1+9]=='3')) document.getElementById(\"tu_lora010eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+10]=='5')||(txt[25+13+1+18+1+10]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+10]=='0')||(txt[25+13+1+18+1+10]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol011\").innerHTML=txt3; if((txt[25+13+1+18+1+10]=='2')||(txt[25+13+1+18+1+10]=='3')) document.getElementById(\"tu_lora011eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+11]=='5')||(txt[25+13+1+18+1+11]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+11]=='0')||(txt[25+13+1+18+1+11]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol012\").innerHTML=txt3; if((txt[25+13+1+18+1+11]=='2')||(txt[25+13+1+18+1+11]=='3')) document.getElementById(\"tu_lora012eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+12]=='5')||(txt[25+13+1+18+1+12]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+12]=='0')||(txt[25+13+1+18+1+12]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol013\").innerHTML=txt3; if((txt[25+13+1+18+1+12]=='2')||(txt[25+13+1+18+1+12]=='3')) document.getElementById(\"tu_lora013eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013eol\").innerHTML=\" \";\r\n"
"if((txt[25+13+1+18+1+13]=='5')||(txt[25+13+1+18+1+13]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+13]=='0')||(txt[25+13+1+18+1+13]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol014\").innerHTML=txt3; if((txt[25+13+1+18+1+13]=='2')||(txt[25+13+1+18+1+13]=='3')) document.getElementById(\"tu_lora014eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014eol\").innerHTML=\" \";\r\n"
"\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp001a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora001a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp001b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora001b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+7+i];txt4[5]=txt[25+13+1+18+1+14+1+0+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+0+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp001c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp002a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora002a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp002b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora002b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+7+i];txt4[5]=txt[25+13+1+18+1+14+1+12+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+12+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp002c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp003a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora003a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp003b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora003b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+7+i];txt4[5]=txt[25+13+1+18+1+14+1+24+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+24+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp003c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp004a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora004a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp004b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora004b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+7+i];txt4[5]=txt[25+13+1+18+1+14+1+36+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+36+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp004c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp005a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp005b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+7+i];txt4[5]=txt[25+13+1+18+1+14+1+48+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+48+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp005c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp006a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp006b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+7+i];txt4[5]=txt[25+13+1+18+1+14+1+60+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+60+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp006c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp007a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp007b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+7+i];txt4[5]=txt[25+13+1+18+1+14+1+72+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+72+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp007c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp008a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp008b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+7+i];txt4[5]=txt[25+13+1+18+1+14+1+84+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+84+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp008c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp009a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp009b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+7+i];txt4[5]=txt[25+13+1+18+1+14+1+96+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+96+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp009c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp010a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp010b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+7+i];txt4[5]=txt[25+13+1+18+1+14+1+108+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+108+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp010c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp011a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora011a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp011b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora011b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+7+i];txt4[5]=txt[25+13+1+18+1+14+1+120+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+120+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp011c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp012a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora012a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp012b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora012b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+7+i];txt4[5]=txt[25+13+1+18+1+14+1+132+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+132+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp012c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp013a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora013a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp013b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora013b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+7+i];txt4[5]=txt[25+13+1+18+1+14+1+144+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+144+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp013c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp014a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora014a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp014b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora014b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+7+i];txt4[5]=txt[25+13+1+18+1+14+1+156+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+156+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp014c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp017a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora017a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp017b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora017b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+7+i];txt4[5]=txt[25+13+1+18+1+14+1+168+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+168+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp017c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp018a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora018a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp018b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora018b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+7+i];txt4[5]=txt[25+13+1+18+1+14+1+180+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+180+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp018c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp019a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora019a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp019b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora019b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+7+i];txt4[5]=txt[25+13+1+18+1+14+1+192+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+192+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp019c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
"  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp020a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora020a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020a\").innerHTML=\" \";\r\n"
" for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp020b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora020b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020b\").innerHTML=\" \";\r\n"
" for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+7+i];txt4[5]=txt[25+13+1+18+1+14+1+204+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+204+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp020c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n"
" \r\n"
" for(i=0;i<5;i++) txt5[i]=txt[25+13+1+18+1+14+1+216+1+20+1+140+1+0+i];for(i=0;i<9;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+0+i];document.getElementById(\"tu_adc015\").innerHTML=txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" \"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4];\r\n"
"for(i=0;i<5;i++) txt5[i]=txt[25+13+1+18+1+14+1+216+1+20+1+140+1+6+i];for(i=0;i<9;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+10+i];document.getElementById(\"tu_adc016\").innerHTML=txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" \"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4];\r\n"
"\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+0+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt001\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+10+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt002\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+20+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt003\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+30+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt004\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+40+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt005\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+50+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt006\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+60+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt007\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+70+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt008\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+80+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt009\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+90+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt010\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+100+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt011\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+110+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt012\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+120+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt013\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+130+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt014\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n"
"\r\n"
"j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_017\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n"
"j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_018\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n"
"j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_019\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n"
"j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_020\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n"
" }\r\n"
" }\r\n"
" xmlhttp.open(\"GET\",\"/TME.txt\", true);\r\n"
" xmlhttp.send();\r\n"
" }}\r\n"
" </script> \r\n"
"    <script>\r\n"
"  var tyt=[];   tyt[0]=\"<li><a href='/nowa'>Panel graficzny LPC</a></li><li><a href='main'>Panel odczytowy</a></li><li><a href='3main'>Konfiguracja portow</a></li><li><a href='lan'>Ustawienia sieci LAN</a></li><li><a href='wifi'>Ustawienia sieci WiFi</a></li><li><a href='gsm'>Ustawienia sieci GSM</a></li><li><a href='czas'>Ustawienia czasu</a></li><li><a href='maskisms'>Numery telefonow</a></li><li><a href='maskimail'>Skrzynki e-mail</a></li><li><a href='/nrf'>Ustawienia LoRa</a></li><li><a href='logic'>Operacje logiczne</a></li><li><a href='rej'>Rejetrator</a></li><li><a href='Rot'>Rotacja</a></li><li><a href='/wykres1'>Wykres pomiarow</a></li><li><a href='/wykres2'>Wykres licznikow</a></li>\";\r\n"
"  document.getElementById(\"tu_menu\").innerHTML=tyt;\r\n"
"  </script>\r\n"
"\r\n"
"  <style>\r\n"
"­iv { margin: 20px; }\r\n"
" #tu_ant {\r\n"
"padding: 15px 12px;\r\n"
"margin-top: 11px;\r\n"
"border-radius: 6px;\r\n"
"\t\tbackground:   url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABkAAAAcCAYAAACUJBTQAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAgY0hSTQAAeiYAAICEAAD6AAAAgOgAAHUwAADqYAAAOpgAABdwnLpRPAAAAAlwSFlzAAAOxAAADsQBlSsOGwAABsFJREFUSEvtVl2sVNUV/vb5n3POzL2DXFDEolRQxKqtQmpbQ6Vogj/EmCCmoQkvVvx56F+I7UsfeaFNSlJjK02QqkmLL6Y19aeNQdT6oKA4pEKsjbfQcJF75947d+bM+dl791tnuNHXvndy150ze6+91vrW+tbaRw1OPWAd5QDWhzIelAUcaCiUXKvAfxTuf0GscrnswCgu84BjqaPkjOhq7slvC+MoDLWB4xYKrq4oQ7hVDkeX1KcDEfOFb3k2Fc9rGDqvVAEDii1RMSBZM9SxRpwY6lrYokLiMwivaMKpXLheSOeyqQmAimC0bgRNdFlhUVqFyndRcjmj2Zy6mQQXBFCeyxWForIoyovIec41CWwmIN/9ri2cWXiRT5spytyiKjyUJkYZLMcnk9PofPgxPjo1ifNT8wwCGGtGGG9FWH3lBG77+vVYPtGgiwyRr1EM56GZkUYzBXIJeoFOjm23KmEq/ABTnxVI0+Vw3aV48aX38Jvn/47JczlmZys41A8CoskIn2mQKil4aI47WHNVjJ07N2DrnV/l2QGG+TQznWM8ipn+AZ0c32FtajHVLXHZyhvx0p9P4Bf7XsPp0z1UqYdhxUS4hE4HaeLCdxV0YaAZZTnPvHM/afkYzpe4+sYWnvjZd7Dh1hWI3RyYXUBU5CMnXTIjTFbgt0+/hV8/dQIDHgjGeHCuxM23XYpN316PdeuWYemyCHEjRNY3mLlQ4IP3zjKYKbz8pxM1r7y2D03Gff+xa7B712Y0h9NISkHywYPWtCewd9/LeGr/v+CMkXZDi1tuaWLvnq1YdXkD40tZQLKvn7EmYs5xkFdMmMt1m+DkP85h3/5X8O77Ug9mPlbYtfMm7HnoK2jqWVKcKLK5DG8dOY8oVTDM+U+f+CYOHXgc69YEiNwZZLNTjEYjjZcg9McRuDHarQTNgOQdnsHNN4zj8DMP4yePrkfkMQzaOPLqR2Qr00lRCye2WU+neOPNU3jymX/iwR2r8MD2OzAzPcUmyhGQcZ9MZjjeOY93jk1iQDq30wauvnIJtnxjLZaOGbQYeT7sIRqL8crrHex/8jR2fW8ttm25CqEdQvU622xcNIAwIHzDHmJRS82+CDC94OLAoaM4/OJZzMyxf0aNDsuapolC03Ow/e7LseeH9yEb/AftCYVBPiTado2iyubYbSROr3OvDfseFrICzYkmev0+WmmMf59TeOjHf0HnZB+GlJWPz1wYoT5FPiwNFPe+fG0TB5/ejbHWZ2Rfv15jPEwrFZWhk5P32LgkEk3PHAFKKMp5s28/++TZs0xDRWo3cP+2Dbhj89d4aIBS5zjy9nE8/8fTJImui33f3Zdi78/vRb/7KSbYrGwWZobzzWVfCVsqQgT7wPUUFLuO5cdNN4xBcW3jpkvwh+d24PFHrsH6tV3cuhG4fl0Pux++Fi8cvger1yT17Lx90xVI4hITaQj0B0DFBpfxQPtqoXOX9XsOvFaDUQ1QMKwkCsn3AGe6Bu1lbRTZNC7hftG36HbnsfK6FejPzkK7LrKMA3Posfkc+MUAfp7B80jzmDXmLNNkG51stf6chdcIkLNIUeKzJyzTRBLAZyEcIjK4cKHEoYMdvHFkGhs3jhHJFsTpgB0/QDtJYPolRw87kTVAQCfsdEmXrdNFFnhS0IJdToNlUWB+ro+QtWkwooJE8Dhlf/dchxPhDD7+NMPvnz2HAwePopWMU4eN2Z2jIUmPEEQz/RmMoHFkDtTZ5ANp67AeMofkHkhjnwziuOefUj4y4+DNY3P1iK+SGEPy5Oj7F9CnusMAwkjGJX+4JEFI+nMAD0ODSqaqNKN+53biIUyBWjsWyCIj5xy06JGzXrQMP/jR6/jbX7vYvHUJfvXLzbD9GYS8yFy5Cesak7hixmN0RCG9knkekdR3qGjIRTW6NrVIDVScOYyBWa0MGoGDtOkgDl3kg4JBisXFACUpn4vEJyhkrPC0OBnZE++GoglTRFBo3pLyCqDl+iXCvMc9EiNu8H2AwdHM6GydhtGzAFs0ygzSiaRlUYeohCEuayBSp5k5l6hch0zj6ZBIXJJikPG+l+O11cUoR8+S7UWjmhn8P5L/sSaf12hUNNbIspGsxztc3r3YsQmv4qChsXJVhMtWR7jiSxHzznnncKQIQUSoJ1eFvORZ+S2fEZM5hd/+lo0duSikWheZMnoVucgYeeZkFi5pjhz5SeMOx0fdH7UsqgrlF8/yu6BwhDk554uWJ76mMny2DB0adjwnmxjVJuA7mNwj3JOJWk8I4bQ7WqNufVbmHMXQhqaU5P8wdNBlg/4X8S+jJn4L8/oAAAAASUVORK5CYII=');        }\r\n"
"</style>\r\n"
"</html>";



typedef struct
{
	char a1[100];
}aaa;
SDRAM static aaa bbb[40];

int IsValueInRangeLowHigh(int val, int low, int high)
{
	if((val>low)&&(val<high))
		return 1;
	else
		return 0;
}



static char *ParserPageHTTP(char *pName)
{
	int i;

	if((strstr(pName,"v_emailSendLogin_")))
	{
		i=atoi(pName+17);
		if(IsValueInRangeLowHigh(i,-1,40))
		{
			return bbb[i].a1;
		}
	}

	return 0;
}

static void ParserQueryHTTP(char *name, char *val)
{
	int i;
	char *pV;

	if((pV=strstr(name,"n_GPIO_val_")))
	{
		i=atoi(pV+11)-1;
		if((i>-1)&&(i<40))
		{
			mini_snprintf(bbb[i].a1,60,"Rafal %02d-%s Mark",atoi("_12_"),val);
		}
	}
	else if((pV=strstr(name,"n_109_zapisz")))
	{
		Dbg(1,"\r\n---ZAPISZ do pamieci----");
	}
}

static uint32_t ASP_Callback( char *dataIn, uint32_t dataInLen, char *dataOut)
{
	char *pS, *pE, *pD=dataIn, *pV, aspName[MAX_SIZE_ASP_NAME];
	int i,j,l,valLen,diff;
	uint32_t n=0;

	for(i=0;i<dataInLen;i++)
	{
		if((*(pD+i)=='[')&&((*(pD+i+1)=='#')))
		{
			pS=pD+i+2;
			if((pE=strstr(pS,"#]")))
			{
				diff=pE-pS;
				if(IsValueInRangeLowHigh(diff,0,MAX_SIZE_ASP_NAME))
				{
					for(j=0;j<diff;j++)
						aspName[j]=*(pS+j);
					aspName[j]=0;

					pV=ParserPageHTTP(aspName);
					if(pV!=0)
					{
						valLen=mini_strlen(pV);
						for(l=0;l<valLen;l++)
							dataOut[n++]=*(pV+l);
						i+=diff+3;
					}
					else
						dataOut[n++]=*(pD+i);
				}
				else
					dataOut[n++]=*(pD+i);
			}
			else
				dataOut[n++]=*(pD+i);
		}
		else
			dataOut[n++]=*(pD+i);
	}
	dataOut[n]=0;
	return n;
}

static void CGI_Callback (char *data)
{
	int j,diff;
	char *pS,*pE, cgiName[MAX_SIZE_CGI_NAME], cgiVal[MAX_SIZE_CGI_VAL];

	if((pS=strstr(data,CGI_GET_STRING)))
	{
		pS += mini_strlen(CGI_GET_STRING);
		while(1)
		{
			if((pE=strstr(pS,"=")))
			{
				diff=pE-pS;
				if(IsValueInRangeLowHigh(diff,0,MAX_SIZE_CGI_NAME))
				{
					for(j=0;j<diff;j++)
						cgiName[j]=*(pS+j);
					cgiName[j]=0;

					pS=pE+1;
					if((pE=strstr(pS,"&")))
					{
						diff=pE-pS;
						if(IsValueInRangeLowHigh(diff,0,MAX_SIZE_CGI_VAL))
						{
							for(j=0;j<diff;j++)
								cgiVal[j]=*(pS+j);
							cgiVal[j]=0;

							ParserQueryHTTP(cgiName,cgiVal);
							pS=pE+1;
						}
						else
							break;
					}
					else
					{
						ParserQueryHTTP(cgiName,"");
						break;
					}
				}
				else
					break;
			}
			else
				break;
		}
	}
}

DATA_TO_SEND* GetPageWWW(char *getHttpRequest)
{
	DATA_TO_SEND *temp = (DATA_TO_SEND*)pvPortMalloc(sizeof(DATA_TO_SEND));
	//char *pIn=dataBufferHTTP_pre;

	temp->state=0;

//	if (STARTUP_WaitForBits(0x0010))  //!!!!!!!!!!!!!! ACCESS_TO_SDCARD !!!!!!!!!!!!!
//	{
//		 STARTUP_ClaerBits(0x0010);
	//if(TakeMutex(Semphr_cardSD, 1000))  //!!!!!!!!! dla Semphr_sdram zawiesza sie lubi !!!!!!!!!!!!!!
	//if(TakeMutex2(Semphr_sdram, Semphr_cardSD, 1000))
	//{

//		temp->pData=GETVAL_ptr(0+HTTP_BUFFER_SIZE);
//
//		if(strstr(getHttpRequest,"GET / ")||strstr(getHttpRequest,"GET /main")){
//
//			if(TakeMutex2(Semphr_sdram, Semphr_cardSD, 1000))
//			//if(TakeMutex(Semphr_cardSD, 1000)
//			{
//				SDCardFileOpen(0,"aaa.htm",FA_READ);
//				temp->len = SDCardFileRead(0, GETVAL_ptr(0), HTTP_BUFFER_SIZE);   // SDCard_ReadFile("aaa.htm", pIn, HTTP_BUFFER_SIZE);  //website/log.htm      //MAX SIZ   ff. FIL INFO!!!!!
//				SDCardFileClose(0);
//				temp->len = ASP_Callback(GETVAL_ptr(0), temp->len, temp->pData);
//				temp->state=1;
//				GiveMutex(Semphr_cardSD);
//				GiveMutex(Semphr_sdram);
//			}
//
//		}

//		else if(strstr(getHttpRequest,"GET /favicon")){
//			SDCardFileOpen(0,"website/favicon.png",FA_READ);
//			temp->len=SDCardFileRead(0, temp->pData, HTTP_BUFFER_SIZE);//SDCard_ReadFile("website/favicon.png", temp->pData, HTTP_BUFFER_SIZE);
//			SDCardFileClose(0);
//		}

//		else if(strstr(getHttpRequest,"GET /logo.png"))
//		{
//			if(strstr(getHttpRequest,"If-None-Match: \"533225b1-12341\""))
//			{
//				Dbg(1,"\r\nXX1111");
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 304 Not Modified\r\nCache-Control: max-age=31536000\r\n\r\n");
//			}
//			else
//			{
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 200 OK\r\nEtag: \"533225b1-12341\"\r\nCache-Control: max-age=31536000\r\n\r\n");
//				SDCardFileOpen(0,"website/logo.png",FA_READ);
//				temp->len += SDCardFileRead(0, temp->pData+temp->len, HTTP_BUFFER_SIZE);//SDCard_ReadFile("website/logo.png", temp->pData+temp->len, HTTP_BUFFER_SIZE);
//				SDCardFileClose(0);
//			}
//		}
//
//		else if(strstr(getHttpRequest,"GET /flags.png"))
//		{
//			if(strstr(getHttpRequest,"If-None-Match: \"533225b1-12341\""))
//			{
//				Dbg(1,"\r\nXX2222");
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 304 Not Modified\r\nCache-Control: max-age=31536000\r\n\r\n");
//			}
//			else
//			{
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 200 OK\r\nEtag: \"533225b1-12341\"\r\nCache-Control: max-age=31536000\r\n\r\n");
//				SDCardFileOpen(0,"website/flags.png",FA_READ);
//				temp->len += SDCardFileRead(0, temp->pData+temp->len, HTTP_BUFFER_SIZE);//SDCard_ReadFile("website/flags.png", temp->pData+temp->len, HTTP_BUFFER_SIZE);
//				SDCardFileClose(0);
//			}
//		}

//		else if(strstr(getHttpRequest,"GET /TME.txt"))
//		{
//			if(TakeMutex(Semphr_sdram, 1000))
//			{
//				temp->len=mini_snprintf(temp->pData,1500,"19:22:05 &nbsp;19/06/13--0000000000000 111111111111111111  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0       7.9       7.7   + 244    1+ 389    1+ 356    1+ 356    1+ 718    1+ 820    1+ 483    1+ 447    1   ---   0   ---   0+ 460    1   ---   0   ---   0   ---   0 V     V              0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0          0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0          0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0          0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0 ");
//				temp->state=1;
//				GiveMutex(Semphr_sdram);
//			}
//		}
//		else{
//			temp=NULL;
//		}
//		else if(strstr(getHttpRequest,CGI_GET_STRING))
//		{
//			CGI_Callback(getHttpRequest);
//			temp->len = mini_snprintf(temp->pData, 200, "HTTP/1.0 200 OK\r\nElektronika RM\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
//			temp->len += mini_snprintf(temp->pData+temp->len, 500, "<html><head><link rel=\"shortcut icon\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABGdBTUEAALGPC/xhBQAAACRJREFUOE9jZGj4z0AaAGogCZGmGuQckowf1UBkcI0GKxEBBQA55GigBl4jdAAAAABJRU5ErkJggg==\"><meta http-equiv=\"refresh\" content=\"0;url=../\"></head><body bgcolor=\"000000\"></body></html>");
//		}
//
//		else if(strstr(getHttpRequest,"GET /LOGIN "))
//			temp->len=mini_snprintf(temp->pData,20,"LOGIN");


		//	STARTUP_SetBits(0x0010);

//	}
//	else
//	{
//		temp->len=0;
//	}



	return temp;
}
