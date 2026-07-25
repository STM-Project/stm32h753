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

/* W NotePad nalezy zamienic '\r\n' -> '\\r\\n\\\r\n' */
char HttpRefr[]="22:59:08 &nbsp;26/07/25120000000000000 111111111111111111 55555555555555  25.10 65%0  78.30 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0       6.0       5.7      ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0 V     V              0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0";

const char HttpBuff[]="<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\"\"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\r\n\
<html>\r\n\
<head>\r\n\
<meta name=\"handheldFriendly\" content=\"true\" />\r\n\
<link rel=\"shortcut icon\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABGdBTUEAALGPC/xhBQAAACRJREFUOE9jZGj4z0AaAGogCZGmGuQckowf1UBkcI0GKxEBBQA55GigBl4jdAAAAABJRU5ErkJggg==\">\r\n\
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
#mainselection select {\r\n\
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
</head>\r\n\
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
function zaladuj()\r\n\
{\r\n\
window.location.href=\"lpc.cgi/n_GPIO_val_005=\" + active_id005[0]+active_id005[1] +\r\n\
\"&n_GPIO_val_006=\" + active_id006[0]+active_id006[1]+\r\n\
\"&n_GPIO_val_007=\" + active_id007[0]+active_id007[1]+\r\n\
\"&n_GPIO_val_008=\" + active_id008[0]+active_id008[1]+\r\n\
\"&n_GPIO_val_009=\" + active_id009[0]+active_id009[1]+\r\n\
\"&n_GPIO_val_010=\" + active_id010[0]+active_id010[1]+\r\n\
\"&n_GPIO_val_028=\" + active_id028[0]+active_id028[1]+\r\n\
\"&n_GPIO_val_021=\" + active_id021[0]+active_id021[1]+\r\n\
\"&n_GPIO_val_022=\" + active_id022[0]+active_id022[1]+\r\n\
\"&n_GPIO_val_023=\" + active_id023[0]+active_id023[1]+\r\n\
\"&n_GPIO_val_024=\" + active_id024[0]+active_id024[1]+\r\n\
\"&n_GPIO_val_033=\" + active_id033[0]+active_id033[1]+\r\n\
\"&n_109_zapisz=\";\r\n\
 }\r\n\
  </script>\r\n\
 \r\n\
 <script>\r\n\
 var nr_pwm; var val_pwm; var i,j,wst;  var txt=[]; \r\n\
 wst=0; \r\n\
 var now=0;void setInterval(function(){now++;if(now==2){wst=0;}},500);\r\n\
 function loadXMLDoc_PWM()\r\n\
 {var xmlhttp;       \r\n\
 if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
 else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n\
 xmlhttp.open(\"GET\",\"/lpc.cgi/n_pwm\"+nr_pwm+\"=\"+val_pwm+\"&n_115_zapisz=\", true);\r\n\
 xmlhttp.send();\r\n\
 }\r\n\
 function loadXMLDoc_PK()\r\n\
 { var xmlhttp;       \r\n\
   if (window.XMLHttpRequest) xmlhttp=new XMLHttpRequest();\r\n\
   else xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");  \r\n\
   xmlhttp.onreadystatechange=function()\r\n\
   { if (xmlhttp.readyState==4 && xmlhttp.status==200){\r\n\
	  txt= xmlhttp.responseText;\r\n\
	  if(txt[0]=='<'){\r\n\
if(nr_pwm==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\r\n\
}\r\n\
}\r\n\
}\r\n\
if(nr_pwm==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\r\n\
if(nr_pwm==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\r\n\
if(nr_pwm==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\r\n\
if(nr_pwm==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\r\n\
if(nr_pwm==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\r\n\
if(nr_pwm==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\r\n\
xmlhttp.send();\r\n\
}\r\n\
function xpk028(){if(wst==0){wst=1;now=0;nr_pwm=28;loadXMLDoc_PK(); }}\r\n\
function xpk021(){if(wst==0){wst=1;now=0;nr_pwm=21;loadXMLDoc_PK(); }}\r\n\
function xpk022(){if(wst==0){wst=1;now=0;nr_pwm=22;loadXMLDoc_PK(); }}\r\n\
function xpk023(){if(wst==0){wst=1;now=0;nr_pwm=23;loadXMLDoc_PK(); }}\r\n\
function xpk024(){if(wst==0){wst=1;now=0;nr_pwm=24;loadXMLDoc_PK(); }}\r\n\
function xpk033(){if(wst==0){wst=1;now=0;nr_pwm=33;loadXMLDoc_PK(); }}\r\n\
function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){if(wst==0){wst=1;now=0;nr_pwm=1; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){if(wst==0){wst=1;now=0;nr_pwm=2; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){if(wst==0){wst=1;now=0;nr_pwm=3; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
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
  var tyt=[];   tyt[0]=\"<li><a href='main'>Panel odczytowy</a></li><li><a href='3main'>Konfiguracja portow</a></li><li><a href='lan'>Ustawienia sieci LAN</a></li><li><a href='wifi'>Ustawienia sieci WiFi</a></li><li><a href='gsm'>Ustawienia sieci GSM</a></li><li><a href='czas'>Ustawienia czasu</a></li><li><a href='maskisms'>Numery telefonow</a></li><li><a href='maskimail'>Skrzynki e-mail</a></li><li><a href='logic'>Operacje logiczne</a></li><li><a href='rej'>Rejetrator</a></li><li><a href='Rot'>Rotacja</a></li>\";\r\n\
  document.getElementById(\"tu_menu\").innerHTML=tyt;\r\n\
  </script>\r\n\
\r\n\
  \r\n\
</html>";


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
