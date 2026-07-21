/*
 * wwwPages.c
 *
 *  Created on: 23.12.2020
 *      Author: RafalMar
 */

#include "wwwPages.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include "SD_Card.h"
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

//SDRAM char dataBufferHTTP[HTTP_BUFFER_SIZE];
//SDRAM char dataBufferHTTP_pre[HTTP_BUFFER_SIZE_PRE];

const char HttpBuff[]="<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\" \"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\
<!-- saved from url=(0023)http://192.168.4.1/main -->\
<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\
<meta name=\"handheldFriendly\" content=\"true\">\
<link rel=\"shortcut icon\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABGdBTUEAALGPC/xhBQAAACRJREFUOE9jZGj4z0AaAGogCZGmGuQckowf1UBkcI0GKxEBBQA55GigBl4jdAAAAABJRU5ErkJggg==\">\
   <style>.c{border-width: 0px;text-decoration: none;color: #222222;font-size: 19px;background-color: #468ccf;border-radius: 10px;padding: 5px 13px 5px 13px;display:inline-block;}</style>\
  <style>.v{border:0px solid #c4c4c4;height:60px;width:74px;font-size:25px;color:#ffffff;background-color:#505050;}</style>\
  <style>.m{border-width: 0px;text-decoration: none;color: #202020;font-size: 20px;background-color: #e0e0e0;border-radius: 10px;padding: 5px 13px 5px 13px;}</style>\
  <style>.n{border-width: 0px;text-decoration: none;color: #202020;font-size: 18px;background-color: #e0e0e0;border-radius: 10px;padding: 5px 13px 5px 13px;}</style>\
  <style>.x{border-radius: 30px;float:left;width: 320px;height: 120px;padding: 1px;border: solid 4px #808080;background-color: #505050;text-align: center;overflow: auto;font-size: 40px;}</style>\
 <style>.y{border-radius: 0px;float:left;width: 350px;height: 150px;text-align: center;overflow: auto;}</style>\
 <style>.b{font-size: 21px;color: #9edf0f;display:inline-block;}</style>\
 <style>.a{font-size: 50px;color: #efefef;display:inline-block;}</style>\
 <style>.z{font-size: 30px;color: #efefef;display:inline-block;}</style>\
<style>\
  .textbox {\
    border: 1px solid #c4c4c4;\
    height: 20px;\
    width: 270px;\
    font-size: 17px;\
    padding: 4px 4px 4px 4px;\
	background-color: #BBBBBB;\
    background: -moz-linear-gradient(top, #FFF, #BBBBBB);\
    background: -webkit-gradient(linear, left top, left bottom, color-stop(0.0, #FFF), color-stop(1.0, #BBBBBB));\
\
    border-radius: 4px;\
    -moz-border-radius: 4px;\
    -webkit-border-radius: 4px;\
    box-shadow: 0px 0px 2px #d9d9d9;\
    -moz-box-shadow: 0px 0px 2px #d9d9d9;\
    -webkit-box-shadow: 0px 0px 2px #d9d9d9;\
	//font-family: Comic Sans MS;\
    font-size: 18px;\
}\
\
.textbox:focus {\
    outline: none;\
    border: 3px solid #7bc1f7;\
    box-shadow: 0px 0px 12px #7bc1f7;\
    -moz-box-shadow: 0px 0px 12px #7bc1f7;\
    -webkit-box-shadow: 0px 0px 12px #7bc1f7;\
}\
 </style>\
<style>\
  .textboxB {\
    border: 1px solid #c4c4c4;\
    color: #fff;\
    height: 20px;\
    font-size: 17px;\
    padding: 4px 4px 4px 4px;\
    border-radius: 4px;\
    font-size: 18px;\
}\
 </style>\
\
 <style>\
\
.shiny-button {\
  display: inline-block;\
  text-align: center;\
  border-width: 1px;\
  border-style: solid;\
  text-transform: uppercase;\
  text-decoration: none;\
  line-height: 1.1;\
  font-weight: normal;\
  font-family: sans-serif;\
  color: #FFFFFF;\
  font-size: 20px;\
  background-color: #73B10E;\
  border-color: #73B10E;\
  -webkit-border-radius: 2px;\
  -moz-border-radius: 2px;\
  border-radius: 6px;\
  padding: 3px 7px 3px 7px;\
}\
</style>\
\
\
<style>\
.css_button {\
    font-size: 23px;\
    font-family:Arial;\
    font-weight:normal;\
    -moz-border-radius: 26px;\
    -webkit-border-radius: 26px;\
    border-radius: 16px;\
    border: 1px solid #3866A3;\
    padding: 9px 25px;\
    text-shadow:1px 1px 0px #5E5E5E;\
    -moz-box-shadow:inset 1px 1px 0px 0px #BEE2F9;\
    -webkit-box-shadow:inset 1px 1px 0px 0px #BEE2F9;\
    box-shadow:inset 1px 1px 0px 0px #BEE2F9;\
    font-weight:normal;\
    text-decoration:none;\
    cursor: pointer;\
    background:-webkit-linear-gradient(90deg, #468ccf, #63b8ee);\
    background:-moz-linear-gradient(90deg, #468ccf, #63b8ee);\
    background:-ms-linear-gradient(90deg, #468ccf, #63b8ee);\
    background:linear-gradient(180deg, #63b8ee, #468ccf);\
    filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#63b8ee', endColorstr='#468ccf');\
    color:#FFFFFF;\
    display:inline-block;\
}\
</style>\
\
\
<style>\
.css-font-style{\
   font-size: 20px;\
   padding: 10px;\
   color: #ff0000;\
   border: solid 0px #000;\
   font-family: Arial, \"Helvetica Neue\", Helvetica, sans-serif;\
}\
</style>\
\
<style>\
.css-font-style2{\
   font-size: 20px;\
   padding: 10px;\
   color: #9edf0f;\
   border: solid 0px #000;\
   font-family: Arial, \"Helvetica Neue\", Helvetica, sans-serif;\
}\
</style>\
<style>\
.css_list{\
   font-size: 20px;\
   padding: 4px;\
   color: #9edf0f;\
   border: solid 0px #000;\
   font-family: Arial, \"Helvetica Neue\", Helvetica, sans-serif;\
}\
</style>\
\
\
\
\
<style>\
.corner-radius{\
    border-radius: 30px;\
    float:left;\
    width: 290px;\
    height: 450px;\
    padding: 1px;\
    border: solid 4px #808080;\
    background-color: #505050;\
    text-align: center;\
    overflow: auto;\
    font-family: Arial,Helvetica,sans-serif;\
    font-size: 40px;\
}\
</style>\
<style>\
.corner-radius3{\
    border-radius: 30px;\
    float:left;\
    width: 290px;\
    height: 540px;\
    padding: 1px;\
    border: solid 4px #808080;\
    background-color: #505050;\
    text-align: center;\
    overflow: auto;\
    font-family: Arial,Helvetica,sans-serif;\
    font-size: 40px;\
}\
</style>\
\
\
\
<style>\
.txt_shadow{\
   text-shadow: 1px 1px 1px #BFBFBF;\
   font-size: 37px;\
   color: #ED7300;\
   font-family: Arial Black, Gadget, sans-serif;\
}\
</style>\
<style>\
.corner-radius2{\
    border-radius: 0px;\
    float:left;\
    width: 320px;\
    height: 480px;\
    text-align: center;\
    overflow: auto;\
}\
</style>\
<style>\
.corner-radius4{\
    border-radius: 0px;\
    float:left;\
    width: 320px;\
    height: 570px;\
    text-align: center;\
    overflow: auto;\
}\
</style>\
<style>\
#holder {\
    width: 100%;\
}\
\
#holder > div {\
    clear: both;\
    padding: 2%;\
    margin-bottom: 20px;\
    border-bottom: 1px solid #eee;\
    float: left;\
    width: 96%;\
}\
\
label {\
    display: inline;\
}\
\
.regular-checkbox {\
    display: none;\
}\
\
.regular-checkbox + label {\
    background-color: #fafafa;\
    border: 1px solid #cacece;\
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05), inset 0px -15px 10px -12px rgba(0, 0, 0, 0.05);\
    padding: 9px;\
    border-radius: 3px;\
    display: inline-block;\
    position: relative;\
}\
.regular-checkbox:checked + label:after {\
 content: \"\";\
 position: absolute;\
 top: 4px;\
 left: 4px;\
 height: 50%;\
 width: 50%;\
 background-color: #090;\
}\
</style>\
<style>\
­iv { margin: 20px; }\
#mainselection select {\
   border: 0;\
   color: #EEE;\
   background: #505050;\
   font-size: 20px;\
   padding: 2px 5px;\
   /*background: #58B14C;\
}\
</style>\
<script>\
	function enableChart()\
	{\
		if( document.getElementById('n_s_Lan_param_001').value == \"nie\")document.getElementById('n_s_Lan_ip_001').disabled=false;\
		else document.getElementById('n_s_Lan_ip_001').disabled = true;\
\
		if( document.getElementById('n_s_Lan_param_001').value == \"nie\")document.getElementById('n_s_Lan_mask_001').disabled=false;\
		else document.getElementById('n_s_Lan_mask_001').disabled = true;\
		\
		if( document.getElementById('n_s_Lan_param_001').value == \"nie\")document.getElementById('n_s_Lan_br_001').disabled=false;\
		else document.getElementById('n_s_Lan_br_001').disabled = true;\
	}\
</script>\
<script>\
\
 function maskujKlawisze(AEvent, AMaska)\
{\
 if (window.Event) {\
  kodKlawisza = AEvent.which;\
 }\
\
 else {\
   kodKlawisza = AEvent.keyCode;\
 }\
 if (kodKlawisza == 13) {\
  return true;  // Enter\
 };\
 if (kodKlawisza == 0) {\
  return true;  // klawisze steruj¹ce\
 };\
 if (kodKlawisza == 8) {\
  return true;  // backspace\
 };\
 klawisz = String.fromCharCode(kodKlawisza);\
 if (AMaska.indexOf(klawisz) == -1) {\
  return false;\
 } else {\
   return true;\
 }\
}\
 function dop_zakr_min_sek(element){\
   if(element.value>59){ alert('wartosc powinna miescic sie w przedziale 0 - 59'); element.value='00'; }\
}\
\
</script>\
 <style> \
.menu {\
    border: none;\
    border: 0px;\
    margin: 0px;\
    padding: 0px;\
    font-family: verdana,geneva,arial,helvetica,sans-serif;\
    font-size: 18px;\
    font-weight: bold;\
    color: 8e8e8e;\
}\
.menu li {\
    float: left;\
    padding: 0px 0px 0px 0px;\
}\
.menu li a {\
    color: #666666;\
    display: block;\
    font-weight: bold;\
    line-height: 43px;\
    padding: 0px 15px;\
    text-align: center;\
    text-decoration: none;\
}\
.menu li a:hover {\
    color: #000000;\
    text-decoration: none;\
}\
.menu li ul {\
    background: #e0e0e0;\
    border-left: 2px solid #a80329;\
    border-right: 2px solid #a80329;\
    border-bottom: 2px solid #a80329;\
    display: none;\
    height: auto;\
    filter: alpha(opacity=95);\
    opacity: 0.95;\
    position: absolute;\
    width: 225px;\
    z-index: 100;\
    \
}\
.menu li:hover ul {\
    display: block;\
}\
.menu li li {\
   // display: block;\
    float: none;\
    padding: 0px;\
    width: 225px;\
}\
.menu li ul a {\
    display: block;\
    font-size: 16px;\
    font-style: normal;\
    padding: 0px 10px 0px 15px;\
    text-align: left;\
}\
.menu li ul a:hover {\
    background: #949494;\
    color: #000000;\
    opacity: 1.0;\
    filter: alpha(opacity=100);\
}\
.menu #current {\
    background: url(data:image/gif;base64,R0lGODlhAQArAJEAALk1VHsMJqgDKQAAACH5BAAAAAAALAAAAAABACsAAAIHFISpy+0aCgA7) top left repeat-x;\
    color: #ffffff;\
}\
</style>\
</head>\
<body bgcolor=\"000000\">\
\
<table><tbody><tr><td>\
	 <div class=\"menu\">\
	 <ul>\
	<li><a href=\"http://192.168.4.1/main#\" id=\"current\">Menu&nbsp;&#9660;</a>\
	   <ul>\
	    <div id=\"tu_menu\"><li><a href=\"http://192.168.4.1/main\">Panel odczytowy</a></li><li><a href=\"http://192.168.4.1/3main\">Konfiguracja portow</a></li><li><a href=\"http://192.168.4.1/lan\">Ustawienia sieci LAN</a></li><li><a href=\"http://192.168.4.1/wifi\">Ustawienia sieci WiFi</a></li><li><a href=\"http://192.168.4.1/gsm\">Ustawienia sieci GSM</a></li><li><a href=\"http://192.168.4.1/czas\">Ustawienia czasu</a></li><li><a href=\"http://192.168.4.1/maskisms\">Numery telefonow</a></li><li><a href=\"http://192.168.4.1/maskimail\">Skrzynki e-mail</a></li><li><a href=\"http://192.168.4.1/logic\">Operacje logiczne</a></li><li><a href=\"http://192.168.4.1/rej\">Rejetrator</a></li><li><a href=\"http://192.168.4.1/Rot\">Rotacja</a></li></div>\
        <br><font size=\"1\" color=\"#777\">Nr ident. &nbsp;&nbsp;1111111111111111</font>\
	   </ul>\
	  </li>\
	<br>\
	</ul>\
	</div>\
	<br>\
	</td>\
	<td width=\"10px\"></td>\
    <td><button class=\"n\"><div id=\"tu_zgr\">02:48:29</div></button></td>\
	<td width=\"10px\"></td>\
	<td><button class=\"n\"><div id=\"tu_gsm\">GSM: --</div></button></td>\
	<td width=\"10px\"></td>\
	<td>                                                                           </td>\
	</tr></tbody></table>\
\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_temp005a\"><font color=\"#eee\">  --.- &deg;C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div><div id=\"tu_lora005a\"> </div>\
<div id=\"tu_temp005b\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora005c\"></div><div id=\"tu_temp005c\" style=\"display: none;\"><font color=\"#eee\">-- %</font></div><div id=\"tu_lora005b\"> </div><div id=\"tu_temp006a\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora006a\"> </div><div id=\"tu_temp006b\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora006c\"></div><div id=\"tu_temp006c\" style=\"display: none;\"><font color=\"#eee\">-- %</font></div><div id=\"tu_lora006b\"> </div><div class=\"y\" style=\"height:300px\"><div class=\"x\" style=\"height:270px\"><font class=\"a\"><b><div id=\"tu_temp007b\"><font color=\"#eee\">  --.- °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora007c\"></div><br>\
<font class=\"a\"><b><div id=\"tu_temp007c\"><font color=\"#eee\">-- %</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font><div id=\"tu_lora007b\"> </div></div></div>\
<div id=\"tu_temp007a\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora007a\"> </div><div id=\"tu_temp008a\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora008a\"> </div><div id=\"tu_temp008b\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora008c\"></div><div id=\"tu_temp008c\" style=\"display: none;\"><font color=\"#eee\">-- %</font></div><div id=\"tu_lora008b\"> </div><div id=\"tu_temp009a\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora009a\"> </div><div id=\"tu_temp009b\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora009c\"></div><div id=\"tu_temp009c\" style=\"display: none;\"><font color=\"#eee\">-- %</font></div><div id=\"tu_lora009b\"> </div><div id=\"tu_temp010a\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora010a\"> </div><div id=\"tu_temp010b\" style=\"display: none;\"><font color=\"#eee\">  --.- °C</font></div><div id=\"tu_lora010c\"></div><div id=\"tu_temp010c\" style=\"display: none;\"><font color=\"#eee\">-- %</font></div><div id=\"tu_lora010b\"> </div>   <div id=\"tu_we005\" style=\"display: none;\"><font color=\"#eee\">Stan OFF             </font></div><div id=\"tu_lora005we\"> </div>\
<div id=\"tu_we006\" style=\"display: none;\"><font color=\"#eee\">Stan OFF             </font></div><div id=\"tu_lora006we\"> </div>\
<div id=\"tu_we007\" style=\"display: none;\"><font color=\"#eee\">Stan OFF             </font></div><div id=\"tu_lora007we\"> </div>\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_we008\"><font color=\"#eee\">Stan OFF             </font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 8</font></div></div><div id=\"tu_lora008we\"> </div>\
<div id=\"tu_we009\" style=\"display: none;\"><font color=\"#eee\">Stan OFF             </font></div><div id=\"tu_lora009we\"> </div>\
<div id=\"tu_we010\" style=\"display: none;\"><font color=\"#eee\">Stan OFF             </font></div><div id=\"tu_lora010we\"> </div>\
<div id=\"tu_eol005\" style=\"display: none;\"><font color=\"#f88\">Uszkodzona linia     </font></div><div id=\"tu_lora005eol\"> </div>\
<div id=\"tu_eol006\" style=\"display: none;\"><font color=\"#f88\">Uszkodzona linia     </font></div><div id=\"tu_lora006eol\"> </div>\
<div id=\"tu_eol007\" style=\"display: none;\"><font color=\"#f88\">Uszkodzona linia     </font></div><div id=\"tu_lora007eol\"> </div>\
<div id=\"tu_eol008\" style=\"display: none;\"><font color=\"#f88\">Uszkodzona linia     </font></div><div id=\"tu_lora008eol\"> </div>\
<div class=\"y\"><div class=\"x\"><font class=\"z\"><b><div id=\"tu_eol009\"><font color=\"#f88\">Uszkodzona linia     </font></div></b></font><br><font class=\"b\">Nazwa dla Wejscia 9</font></div></div><div id=\"tu_lora009eol\"> </div>\
<div id=\"tu_eol010\" style=\"display: none;\"><font color=\"#f88\">Uszkodzona linia     </font></div><div id=\"tu_lora010eol\"> </div>\
<div id=\"tu_ptt005\" style=\"display: none;\"><font color=\"#eee\">   ---    °C</font></div>\
<div class=\"y\"><div class=\"x\"><font class=\"a\"><b><div id=\"tu_ptt006\"><font color=\"#eee\">   ---    °C</font></div></b></font><br><font class=\"b\">Nazwa czujnika</font></div></div>\
<div id=\"tu_ptt007\" style=\"display: none;\"><font color=\"#eee\">   ---    °C</font></div>\
<div id=\"tu_ptt008\" style=\"display: none;\"><font color=\"#eee\">   ---    °C</font></div>\
<div id=\"tu_ptt009\" style=\"display: none;\"><font color=\"#eee\">   ---    °C</font></div>\
<div id=\"tu_ptt010\" style=\"display: none;\"><font color=\"#eee\">   ---    °C</font></div>\
         <div class=\"y\"><div class=\"x\"><table><tbody><tr><td width=\"100px\"><output class=\"v\" style=\"font-size:36px;\" id=\"volume023\">27</output></td><td></td><td><input type=\"range\" name=\"n_pwm3\" min=\"0\" max=\"100\" step=\"1\" value=\"74\" oninput=\"outputUpdate023(value)\" onchange=\"x023(this.value);\"></td></tr></tbody></table><font class=\"b\">Nazwa portu nr 23     PW3</font></div></div>\
  <div class=\"y\"><div class=\"x\"><a href=\"javascript:return false;\" rel=\"external\" onclick=\"xpk028(); this.href=&#39;javascript:return false;&#39;;\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk028\"><font color=\"#000\">Stan OFF             </font></div></b></a><br><font class=\"b\">Wyjscie Przekaznikowe</font></div></div><div id=\"tu_lora028wy\"> </div>\
<div id=\"tu_pk021\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div><div id=\"tu_lora021wy\"> </div>\
<div id=\"tu_pk022\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div><div id=\"tu_lora022wy\"> </div>\
<div id=\"tu_pk023\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div><div id=\"tu_lora023wy\"> </div>\
<div class=\"y\"><div class=\"x\"><a href=\"http://192.168.4.1/main#\" rel=\"external\" onclick=\"xpk024(); this.href=&#39;javascript:return false;&#39;;\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk024\"><font color=\"#000\">Stan OFF             </font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR</font></div></div><div id=\"tu_lora024wy\"> </div>\
<div class=\"y\"><div class=\"x\"><a href=\"http://192.168.4.1/main#\" rel=\"external\" onclick=\"xpk033(); this.href=&#39;javascript:return false;&#39;;\" class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_pk033\"><font color=\"#000\">Stan OFF             </font></div></b></a><br><font class=\"b\">Wyjscie Przekaznikowe</font></div></div><div id=\"tu_lora033wy\"> </div>\
<div id=\"tu_ti028\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_ti021\"><font color=\"#000\">Stan OFF             </font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR,PWM</font></div></div>\
<div id=\"tu_ti022\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_ti023\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_ti024\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_ti033\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_te028\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_te021\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div class=\"y\"><div class=\"x\"><a class=\"c\" style=\"font-size: 25px;\"><b><div id=\"tu_te022\"><font color=\"#000\">Stan OFF             </font></div></b></a><br><font class=\"b\">Wyjscie typu OC,IR,PWM</font></div></div>\
<div id=\"tu_te023\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_te024\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
<div id=\"tu_te033\" style=\"display: none;\"><font color=\"#000\">Stan OFF             </font></div>\
\
\
\
<script type=\"text/javascript\">\
window.onload = function(){\
Laduj();\
pageScroll();\
 };\
var active_id005=\"02p005\";\
var active_id006=\"03p006\";\
var active_id007=\"09p007\";\
var active_id008=\"04p008\";\
var active_id009=\"13p009\";\
var active_id010=\"06p010\";\
var active_id028=\"00p028\";\
var active_id021=\"10p021\";\
var active_id022=\"11p022\";\
var active_id023=\"01p023\";\
var active_id024=\"08p024\";\
var active_id033=\"00p033\";\
function Laduj()\
{\
var blok2 = document.getElementById(active_id005);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id006);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id007);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id008);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id009);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id010);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id028);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id021);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id022);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id023);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id024);\
blok2.style.display = \"block\";\
var blok2 = document.getElementById(active_id033);\
blok2.style.display = \"block\";\
\
document.forms['wybor005'].n_GPIO_val_005.onchange = Zmien005;\
document.forms['wybor006'].n_GPIO_val_006.onchange = Zmien006;\
document.forms['wybor007'].n_GPIO_val_007.onchange = Zmien007;\
document.forms['wybor008'].n_GPIO_val_008.onchange = Zmien008;\
document.forms['wybor009'].n_GPIO_val_009.onchange = Zmien009;\
document.forms['wybor010'].n_GPIO_val_010.onchange = Zmien010;\
document.forms['wybor028'].n_GPIO_val_028.onchange = Zmien028;\
document.forms['wybor021'].n_GPIO_val_021.onchange = Zmien021;\
document.forms['wybor022'].n_GPIO_val_022.onchange = Zmien022;\
document.forms['wybor023'].n_GPIO_val_023.onchange = Zmien023;\
document.forms['wybor024'].n_GPIO_val_024.onchange = Zmien024;\
document.forms['wybor033'].n_GPIO_val_033.onchange = Zmien033;\
}\
 function Zmien005()\
 {\
 if (active_id005 != \"\")\
 {\
 var blok2 = document.getElementById(active_id005);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id005 = this.value;\
 }\
 function Zmien006()\
 {\
 if (active_id006 != \"\")\
 {\
 var blok2 = document.getElementById(active_id006);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id006 = this.value;\
 }\
 function Zmien007()\
 {\
 if (active_id007 != \"\")\
 {\
 var blok2 = document.getElementById(active_id007);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id007 = this.value;\
 }\
 function Zmien008()\
 {\
 if (active_id008 != \"\")\
 {\
 var blok2 = document.getElementById(active_id008);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id008 = this.value;\
 }\
 function Zmien009()\
 {\
 if (active_id009 != \"\")\
 {\
 var blok2 = document.getElementById(active_id009);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id009 = this.value;\
 }\
 function Zmien010()\
 {\
 if (active_id010 != \"\")\
 {\
 var blok2 = document.getElementById(active_id010);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id010 = this.value;\
 }\
 function Zmien028()\
 {\
 if (active_id028 != \"\")\
 {\
 var blok2 = document.getElementById(active_id028);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id028 = this.value;\
 }\
 function Zmien021()\
 {\
 if (active_id021 != \"\")\
 {\
 var blok2 = document.getElementById(active_id021);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id021 = this.value;\
 }\
 function Zmien022()\
 {\
 if (active_id022 != \"\")\
 {\
 var blok2 = document.getElementById(active_id022);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id022 = this.value;\
 }\
 function Zmien023()\
 {\
 if (active_id023 != \"\")\
 {\
 var blok2 = document.getElementById(active_id023);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id023 = this.value;\
 }\
 function Zmien024()\
 {\
 if (active_id024 != \"\")\
 {\
 var blok2 = document.getElementById(active_id024);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id024 = this.value;\
 }\
 function Zmien033()\
 {\
 if (active_id033 != \"\")\
 {\
 var blok2 = document.getElementById(active_id033);\
 blok2.style.display = \"none\";\
 }\
 var blok = document.getElementById(this.value);\
 blok.style.display = \"block\";\
 active_id033 = this.value;\
 }\
function zaladuj()\
{\
window.location.href=\"lpc.cgi/n_GPIO_val_005=\" + active_id005[0]+active_id005[1] +\
\"&n_GPIO_val_006=\" + active_id006[0]+active_id006[1]+\
\"&n_GPIO_val_007=\" + active_id007[0]+active_id007[1]+\
\"&n_GPIO_val_008=\" + active_id008[0]+active_id008[1]+\
\"&n_GPIO_val_009=\" + active_id009[0]+active_id009[1]+\
\"&n_GPIO_val_010=\" + active_id010[0]+active_id010[1]+\
\"&n_GPIO_val_028=\" + active_id028[0]+active_id028[1]+\
\"&n_GPIO_val_021=\" + active_id021[0]+active_id021[1]+\
\"&n_GPIO_val_022=\" + active_id022[0]+active_id022[1]+\
\"&n_GPIO_val_023=\" + active_id023[0]+active_id023[1]+\
\"&n_GPIO_val_024=\" + active_id024[0]+active_id024[1]+\
\"&n_GPIO_val_033=\" + active_id033[0]+active_id033[1]+\
\"&n_109_zapisz=\";\
 }\
  </script>\
\
 <script>\
 var nr_pwm; var val_pwm; var i,j,wst;  var txt=[];\
 wst=0;\
 var now=0;void setInterval(function(){now++;if(now==2){wst=0;}},500);\
 function loadXMLDoc_PWM()\
 {var xmlhttp;\
 if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\
 else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \
 xmlhttp.open(\"GET\",\"/lpc.cgi/n_pwm\"+nr_pwm+\"=\"+val_pwm+\"&n_115_zapisz=\", true);\
 xmlhttp.send();\
 }\
 function loadXMLDoc_PK()\
 { var xmlhttp;\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\"); \
   xmlhttp.onreadystatechange=function()\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200){\
	  txt= xmlhttp.responseText;\
	  if(txt[0]=='<'){\
if(nr_pwm==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\
if(nr_pwm==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\
if(nr_pwm==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\
if(nr_pwm==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\
if(nr_pwm==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\
if(nr_pwm==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\
}\
}\
}\
if(nr_pwm==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\
if(nr_pwm==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\
if(nr_pwm==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\
if(nr_pwm==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\
if(nr_pwm==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\
if(nr_pwm==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\
xmlhttp.send();\
}\
function xpk028(){if(wst==0){wst=1;now=0;nr_pwm=28;loadXMLDoc_PK(); }}\
function xpk021(){if(wst==0){wst=1;now=0;nr_pwm=21;loadXMLDoc_PK(); }}\
function xpk022(){if(wst==0){wst=1;now=0;nr_pwm=22;loadXMLDoc_PK(); }}\
function xpk023(){if(wst==0){wst=1;now=0;nr_pwm=23;loadXMLDoc_PK(); }}\
function xpk024(){if(wst==0){wst=1;now=0;nr_pwm=24;loadXMLDoc_PK(); }}\
function xpk033(){if(wst==0){wst=1;now=0;nr_pwm=33;loadXMLDoc_PK(); }}\
function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){if(wst==0){wst=1;now=0;nr_pwm=1; val_pwm=val; loadXMLDoc_PWM();}}\
function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){if(wst==0){wst=1;now=0;nr_pwm=2; val_pwm=val; loadXMLDoc_PWM();}}\
function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){if(wst==0){wst=1;now=0;nr_pwm=3; val_pwm=val; loadXMLDoc_PWM();}}\
var myVar = setInterval(loadXMLDoc, 5000       );\
 var txt=[];\
 var txt1=[];\
 var txt2=[];\
 var txt3=[];\
 var txt4=[];\
 var txt5=[];\
 var ccol=[];\
 function loadXMLDoc()\
 {if(wst==0){wst=1;       now=0;\
   var xmlhttp;\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");\
   xmlhttp.onreadystatechange=function()\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200)\
     {\
		 txt= xmlhttp.responseText;\
		 txt3=txt[0]+txt[1]+txt[2]+txt[3]+txt[4]+txt[5]+txt[6]+txt[7]; document.getElementById(\"tu_zgr\").innerHTML=txt3;\
		 txt3=\"GSM: \"+txt[23]+txt[24]; document.getElementById(\"tu_gsm\").innerHTML=txt3;\
if((txt[25+0]=='0')||(txt[25+0]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk028\").innerHTML=txt3; document.getElementById(\"tu_ti028\").innerHTML=txt3; document.getElementById(\"tu_te028\").innerHTML=txt3;   if((txt[25+0]=='2')||(txt[25+0]=='3')) document.getElementById(\"tu_lora028wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora028wy\").innerHTML=\" \";\
if((txt[25+10]=='0')||(txt[25+10]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk021\").innerHTML=txt3; document.getElementById(\"tu_ti021\").innerHTML=txt3; document.getElementById(\"tu_te021\").innerHTML=txt3;   if((txt[25+10]=='2')||(txt[25+10]=='3')) document.getElementById(\"tu_lora021wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora021wy\").innerHTML=\" \";\
if((txt[25+11]=='0')||(txt[25+11]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk022\").innerHTML=txt3; document.getElementById(\"tu_ti022\").innerHTML=txt3; document.getElementById(\"tu_te022\").innerHTML=txt3;   if((txt[25+11]=='2')||(txt[25+11]=='3')) document.getElementById(\"tu_lora022wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora022wy\").innerHTML=\" \";\
if((txt[25+12]=='0')||(txt[25+12]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk023\").innerHTML=txt3; document.getElementById(\"tu_ti023\").innerHTML=txt3; document.getElementById(\"tu_te023\").innerHTML=txt3;   if((txt[25+12]=='2')||(txt[25+12]=='3')) document.getElementById(\"tu_lora023wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora023wy\").innerHTML=\" \";\
if((txt[25+6]=='0')||(txt[25+6]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk024\").innerHTML=txt3; document.getElementById(\"tu_ti024\").innerHTML=txt3; document.getElementById(\"tu_te024\").innerHTML=txt3;   if((txt[25+6]=='2')||(txt[25+6]=='3')) document.getElementById(\"tu_lora024wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora024wy\").innerHTML=\" \";\
if((txt[25+5]=='0')||(txt[25+5]=='2')) txt3=\"<font color='#000'>Stan OFF             </font>\"; else txt3=\"<font color='#eee'>Stan ON              </font>\";  document.getElementById(\"tu_pk033\").innerHTML=txt3; document.getElementById(\"tu_ti033\").innerHTML=txt3; document.getElementById(\"tu_te033\").innerHTML=txt3;   if((txt[25+5]=='2')||(txt[25+5]=='3')) document.getElementById(\"tu_lora033wy\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora033wy\").innerHTML=\" \";\
\
if((txt[25+13+1+4]=='0')||(txt[25+13+1+4]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we005\").innerHTML=txt3; if((txt[25+13+1+4]=='2')||(txt[25+13+1+4]=='3')) document.getElementById(\"tu_lora005we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005we\").innerHTML=\" \";\
if((txt[25+13+1+5]=='0')||(txt[25+13+1+5]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we006\").innerHTML=txt3; if((txt[25+13+1+5]=='2')||(txt[25+13+1+5]=='3')) document.getElementById(\"tu_lora006we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006we\").innerHTML=\" \";\
if((txt[25+13+1+6]=='0')||(txt[25+13+1+6]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we007\").innerHTML=txt3; if((txt[25+13+1+6]=='2')||(txt[25+13+1+6]=='3')) document.getElementById(\"tu_lora007we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007we\").innerHTML=\" \";\
if((txt[25+13+1+7]=='0')||(txt[25+13+1+7]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we008\").innerHTML=txt3; if((txt[25+13+1+7]=='2')||(txt[25+13+1+7]=='3')) document.getElementById(\"tu_lora008we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008we\").innerHTML=\" \";\
if((txt[25+13+1+8]=='0')||(txt[25+13+1+8]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we009\").innerHTML=txt3; if((txt[25+13+1+8]=='2')||(txt[25+13+1+8]=='3')) document.getElementById(\"tu_lora009we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009we\").innerHTML=\" \";\
if((txt[25+13+1+9]=='0')||(txt[25+13+1+9]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_we010\").innerHTML=txt3; if((txt[25+13+1+9]=='2')||(txt[25+13+1+9]=='3')) document.getElementById(\"tu_lora010we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010we\").innerHTML=\" \";\
\
if((txt[25+13+1+18+1+4]=='5')||(txt[25+13+1+18+1+4]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+4]=='0')||(txt[25+13+1+18+1+4]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol005\").innerHTML=txt3; if((txt[25+13+1+18+1+4]=='2')||(txt[25+13+1+18+1+4]=='3')) document.getElementById(\"tu_lora005eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005eol\").innerHTML=\" \";\
if((txt[25+13+1+18+1+5]=='5')||(txt[25+13+1+18+1+5]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+5]=='0')||(txt[25+13+1+18+1+5]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol006\").innerHTML=txt3; if((txt[25+13+1+18+1+5]=='2')||(txt[25+13+1+18+1+5]=='3')) document.getElementById(\"tu_lora006eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006eol\").innerHTML=\" \";\
if((txt[25+13+1+18+1+6]=='5')||(txt[25+13+1+18+1+6]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+6]=='0')||(txt[25+13+1+18+1+6]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol007\").innerHTML=txt3; if((txt[25+13+1+18+1+6]=='2')||(txt[25+13+1+18+1+6]=='3')) document.getElementById(\"tu_lora007eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007eol\").innerHTML=\" \";\
if((txt[25+13+1+18+1+7]=='5')||(txt[25+13+1+18+1+7]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+7]=='0')||(txt[25+13+1+18+1+7]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol008\").innerHTML=txt3; if((txt[25+13+1+18+1+7]=='2')||(txt[25+13+1+18+1+7]=='3')) document.getElementById(\"tu_lora008eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008eol\").innerHTML=\" \";\
if((txt[25+13+1+18+1+8]=='5')||(txt[25+13+1+18+1+8]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+8]=='0')||(txt[25+13+1+18+1+8]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol009\").innerHTML=txt3; if((txt[25+13+1+18+1+8]=='2')||(txt[25+13+1+18+1+8]=='3')) document.getElementById(\"tu_lora009eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009eol\").innerHTML=\" \";\
if((txt[25+13+1+18+1+9]=='5')||(txt[25+13+1+18+1+9]=='6')) txt3=\"<font color='#f88'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+9]=='0')||(txt[25+13+1+18+1+9]=='2')) txt3=\"<font color='#f88'>Stan ON              </font>\"; else txt3=\"<font color='#eee'>Stan OFF             </font>\";  document.getElementById(\"tu_eol010\").innerHTML=txt3; if((txt[25+13+1+18+1+9]=='2')||(txt[25+13+1+18+1+9]=='3')) document.getElementById(\"tu_lora010eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010eol\").innerHTML=\" \";\
\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005a\").innerHTML=\" \";\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005b\").innerHTML=\" \";\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+7+i];txt4[5]=txt[25+13+1+18+1+14+1+48+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+48+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp005c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\
\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006a\").innerHTML=\" \";\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006b\").innerHTML=\" \";\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+7+i];txt4[5]=txt[25+13+1+18+1+14+1+60+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+60+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp006c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\
\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007a\").innerHTML=\" \";\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007b\").innerHTML=\" \";\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+7+i];txt4[5]=txt[25+13+1+18+1+14+1+72+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+72+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp007c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\
\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008a\").innerHTML=\" \";\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008b\").innerHTML=\" \";\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+7+i];txt4[5]=txt[25+13+1+18+1+14+1+84+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+84+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp008c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\
\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009a\").innerHTML=\" \";\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009b\").innerHTML=\" \";\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+7+i];txt4[5]=txt[25+13+1+18+1+14+1+96+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+96+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp009c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\
\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010a\").innerHTML=\" \";\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010b\").innerHTML=\" \";\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+7+i];txt4[5]=txt[25+13+1+18+1+14+1+108+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+108+7+2]; if(txt4[5]=='1') ccol=\"f88\";if(txt4[5]=='0') ccol=\"eee\";if(txt4[5]=='2') ccol=\"8cf\"; document.getElementById(\"tu_temp010c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\
\
\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+40+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt005\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+50+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt006\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+60+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt007\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+70+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt008\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+80+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt009\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+90+1+20+i];if(txt4[9]=='1') ccol=\"f88\";if(txt4[9]=='0') ccol=\"eee\";if(txt4[9]=='2') ccol=\"8cf\";document.getElementById(\"tu_ptt010\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\
\
 }\
 }\
 xmlhttp.open(\"GET\",\"/TME.txt\", true);\
 xmlhttp.send();\
 }}\
 </script>\
    <script>\
  var tyt=[];   tyt[0]=\"<li><a href='main'>Panel odczytowy</a></li><li><a href='3main'>Konfiguracja portow</a></li><li><a href='lan'>Ustawienia sieci LAN</a></li><li><a href='wifi'>Ustawienia sieci WiFi</a></li><li><a href='gsm'>Ustawienia sieci GSM</a></li><li><a href='czas'>Ustawienia czasu</a></li><li><a href='maskisms'>Numery telefonow</a></li><li><a href='maskimail'>Skrzynki e-mail</a></li><li><a href='logic'>Operacje logiczne</a></li><li><a href='rej'>Rejetrator</a></li><li><a href='Rot'>Rotacja</a></li>\";\
  document.getElementById(\"tu_menu\").innerHTML=tyt;\
  </script>\
\
\
</body></html>";


typedef struct
{
	char a1[100];
}aaa;
SDRAM static aaa bbb[40];

void ResetTestTab(void)  //do usuniecia !!!
{
//	for(int i=0;i<40;i++)
//	{
//		bbb[i].a1[0]=0;
//	}
}

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
