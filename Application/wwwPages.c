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

/* W NotePad nalezy zamienic '\r\n' -> '\\r\\n\\\r\n' */
char HttpRefr[]="22:59:08 &nbsp;26/07/25120000000000000 111111111111111111 55555555555555  25.10 65%0  78.30 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0       6.0       5.7      ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0 V     V              0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0";

const char HttpStyle[]="<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\"\"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\r\n\
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
</head>";

const char HttpMainReadPanel[]="<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\"\"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\r\n\
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
</body>\r\n\
<script type=\"text/javascript\">\r\n\
window.onload = function(){\r\n\
Laduj();\r\n\
pageScroll();\r\n\
 };\r\n\
var active_id001=\"02p001\";\r\n\
var active_id002=\"02p002\";\r\n\
var active_id003=\"03p003\";\r\n\
var active_id004=\"03p004\";\r\n\
var active_id005=\"09p005\";\r\n\
var active_id006=\"09p006\";\r\n\
var active_id007=\"04p007\";\r\n\
var active_id008=\"04p008\";\r\n\
var active_id009=\"13p009\";\r\n\
var active_id010=\"13p010\";\r\n\
var active_id011=\"06p011\";\r\n\
var active_id012=\"06p012\";\r\n\
var active_id013=\"06p013\";\r\n\
var active_id014=\"02p014\";\r\n\
var active_id015=\"05p015\";\r\n\
var active_id016=\"05p016\";\r\n\
var active_id017=\"07p017\";\r\n\
var active_id018=\"07p018\";\r\n\
var active_id019=\"02p019\";\r\n\
var active_id020=\"09p020\";\r\n\
var active_id021=\"00p021\";\r\n\
var active_id022=\"00p022\";\r\n\
var active_id023=\"10p023\";\r\n\
var active_id024=\"10p024\";\r\n\
var active_id025=\"11p025\";\r\n\
var active_id026=\"11p026\";\r\n\
var active_id027=\"08p027\";\r\n\
var active_id028=\"00p028\";\r\n\
var active_id029=\"10p029\";\r\n\
var active_id030=\"11p030\";\r\n\
var active_id031=\"06p031\";\r\n\
var active_id032=\"06p032\";\r\n\
var active_id033=\"00p033\";\r\n\
function Laduj()\r\n\
{\r\n\
var blok2 = document.getElementById(active_id001);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id002);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id003);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id004);\r\n\
blok2.style.display = \"block\";\r\n\
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
var blok2 = document.getElementById(active_id011);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id012);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id013);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id014);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id015);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id016);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id017);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id018);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id019);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id020);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id021);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id022);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id023);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id024);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id025);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id026);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id027);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id028);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id029);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id030);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id031);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id032);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id033);\r\n\
blok2.style.display = \"block\";\r\n\
\r\n\
document.forms['wybor001'].n_GPIO_val_001.onchange = Zmien001;\r\n\
document.forms['wybor002'].n_GPIO_val_002.onchange = Zmien002;\r\n\
document.forms['wybor003'].n_GPIO_val_003.onchange = Zmien003;\r\n\
document.forms['wybor004'].n_GPIO_val_004.onchange = Zmien004;\r\n\
document.forms['wybor005'].n_GPIO_val_005.onchange = Zmien005;\r\n\
document.forms['wybor006'].n_GPIO_val_006.onchange = Zmien006;\r\n\
document.forms['wybor007'].n_GPIO_val_007.onchange = Zmien007;\r\n\
document.forms['wybor008'].n_GPIO_val_008.onchange = Zmien008;\r\n\
document.forms['wybor009'].n_GPIO_val_009.onchange = Zmien009;\r\n\
document.forms['wybor010'].n_GPIO_val_010.onchange = Zmien010;\r\n\
document.forms['wybor011'].n_GPIO_val_011.onchange = Zmien011;\r\n\
document.forms['wybor012'].n_GPIO_val_012.onchange = Zmien012;\r\n\
document.forms['wybor013'].n_GPIO_val_013.onchange = Zmien013;\r\n\
document.forms['wybor014'].n_GPIO_val_014.onchange = Zmien014;\r\n\
document.forms['wybor015'].n_GPIO_val_015.onchange = Zmien015;\r\n\
document.forms['wybor016'].n_GPIO_val_016.onchange = Zmien016;\r\n\
document.forms['wybor017'].n_GPIO_val_017.onchange = Zmien017;\r\n\
document.forms['wybor018'].n_GPIO_val_018.onchange = Zmien018;\r\n\
document.forms['wybor019'].n_GPIO_val_019.onchange = Zmien019;\r\n\
document.forms['wybor020'].n_GPIO_val_020.onchange = Zmien020;\r\n\
document.forms['wybor021'].n_GPIO_val_021.onchange = Zmien021;\r\n\
document.forms['wybor022'].n_GPIO_val_022.onchange = Zmien022;\r\n\
document.forms['wybor023'].n_GPIO_val_023.onchange = Zmien023;\r\n\
document.forms['wybor024'].n_GPIO_val_024.onchange = Zmien024;\r\n\
document.forms['wybor025'].n_GPIO_val_025.onchange = Zmien025;\r\n\
document.forms['wybor026'].n_GPIO_val_026.onchange = Zmien026;\r\n\
document.forms['wybor027'].n_GPIO_val_027.onchange = Zmien027;\r\n\
document.forms['wybor028'].n_GPIO_val_028.onchange = Zmien028;\r\n\
document.forms['wybor029'].n_GPIO_val_029.onchange = Zmien029;\r\n\
document.forms['wybor030'].n_GPIO_val_030.onchange = Zmien030;\r\n\
document.forms['wybor031'].n_GPIO_val_031.onchange = Zmien031;\r\n\
document.forms['wybor032'].n_GPIO_val_032.onchange = Zmien032;\r\n\
document.forms['wybor033'].n_GPIO_val_033.onchange = Zmien033;\r\n\
}\r\n\
 function Zmien001()\r\n\
 {\r\n\
 if (active_id001 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id001);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id001 = this.value;\r\n\
 }\r\n\
 function Zmien002()\r\n\
 {\r\n\
 if (active_id002 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id002);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id002 = this.value;\r\n\
 }\r\n\
 function Zmien003()\r\n\
 {\r\n\
 if (active_id003 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id003);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id003 = this.value;\r\n\
 }\r\n\
 function Zmien004()\r\n\
 {\r\n\
 if (active_id004 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id004);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id004 = this.value;\r\n\
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
 function Zmien011()\r\n\
 {\r\n\
 if (active_id011 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id011);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id011 = this.value;\r\n\
 }\r\n\
 function Zmien012()\r\n\
 {\r\n\
 if (active_id012 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id012);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id012 = this.value;\r\n\
 }\r\n\
 function Zmien013()\r\n\
 {\r\n\
 if (active_id013 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id013);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id013 = this.value;\r\n\
 }\r\n\
 function Zmien014()\r\n\
 {\r\n\
 if (active_id014 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id014);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id014 = this.value;\r\n\
 }\r\n\
 function Zmien015()\r\n\
 {\r\n\
 if (active_id015 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id015);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id015 = this.value;\r\n\
 }\r\n\
 function Zmien016()\r\n\
 {\r\n\
 if (active_id016 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id016);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id016 = this.value;\r\n\
 }\r\n\
 function Zmien017()\r\n\
 {\r\n\
 if (active_id017 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id017);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id017 = this.value;\r\n\
 }\r\n\
 function Zmien018()\r\n\
 {\r\n\
 if (active_id018 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id018);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id018 = this.value;\r\n\
 }\r\n\
 function Zmien019()\r\n\
 {\r\n\
 if (active_id019 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id019);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id019 = this.value;\r\n\
 }\r\n\
 function Zmien020()\r\n\
 {\r\n\
 if (active_id020 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id020);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id020 = this.value;\r\n\
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
 function Zmien025()\r\n\
 {\r\n\
 if (active_id025 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id025);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id025 = this.value;\r\n\
 }\r\n\
 function Zmien026()\r\n\
 {\r\n\
 if (active_id026 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id026);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id026 = this.value;\r\n\
 }\r\n\
 function Zmien027()\r\n\
 {\r\n\
 if (active_id027 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id027);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id027 = this.value;\r\n\
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
 function Zmien029()\r\n\
 {\r\n\
 if (active_id029 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id029);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id029 = this.value;\r\n\
 }\r\n\
 function Zmien030()\r\n\
 {\r\n\
 if (active_id030 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id030);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id030 = this.value;\r\n\
 }\r\n\
 function Zmien031()\r\n\
 {\r\n\
 if (active_id031 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id031);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id031 = this.value;\r\n\
 }\r\n\
 function Zmien032()\r\n\
 {\r\n\
 if (active_id032 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id032);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id032 = this.value;\r\n\
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
window.location.href=\"lpc.cgi/n_GPIO_val_001=\" + active_id001[0]+active_id001[1] +\r\n\
\"&n_GPIO_val_002=\" + active_id002[0]+active_id002[1]+\r\n\
\"&n_GPIO_val_003=\" + active_id003[0]+active_id003[1]+\r\n\
\"&n_GPIO_val_004=\" + active_id004[0]+active_id004[1]+\r\n\
\"&n_GPIO_val_005=\" + active_id005[0]+active_id005[1]+\r\n\
\"&n_GPIO_val_006=\" + active_id006[0]+active_id006[1]+\r\n\
\"&n_GPIO_val_007=\" + active_id007[0]+active_id007[1]+\r\n\
\"&n_GPIO_val_008=\" + active_id008[0]+active_id008[1]+\r\n\
\"&n_GPIO_val_009=\" + active_id009[0]+active_id009[1]+\r\n\
\"&n_GPIO_val_010=\" + active_id010[0]+active_id010[1]+\r\n\
\"&n_GPIO_val_011=\" + active_id011[0]+active_id011[1]+\r\n\
\"&n_GPIO_val_012=\" + active_id012[0]+active_id012[1]+\r\n\
\"&n_GPIO_val_013=\" + active_id013[0]+active_id013[1]+\r\n\
\"&n_GPIO_val_014=\" + active_id014[0]+active_id014[1]+\r\n\
\"&n_GPIO_val_015=\" + active_id015[0]+active_id015[1]+\r\n\
\"&n_GPIO_val_016=\" + active_id016[0]+active_id016[1]+\r\n\
\"&n_GPIO_val_017=\" + active_id017[0]+active_id017[1]+\r\n\
\"&n_GPIO_val_018=\" + active_id018[0]+active_id018[1]+\r\n\
\"&n_GPIO_val_019=\" + active_id019[0]+active_id019[1]+\r\n\
\"&n_GPIO_val_020=\" + active_id020[0]+active_id020[1]+\r\n\
\"&n_GPIO_val_021=\" + active_id021[0]+active_id021[1]+\r\n\
\"&n_GPIO_val_022=\" + active_id022[0]+active_id022[1]+\r\n\
\"&n_GPIO_val_023=\" + active_id023[0]+active_id023[1]+\r\n\
\"&n_GPIO_val_024=\" + active_id024[0]+active_id024[1]+\r\n\
\"&n_GPIO_val_025=\" + active_id025[0]+active_id025[1]+\r\n\
\"&n_GPIO_val_026=\" + active_id026[0]+active_id026[1]+\r\n\
\"&n_GPIO_val_027=\" + active_id027[0]+active_id027[1]+\r\n\
\"&n_GPIO_val_028=\" + active_id028[0]+active_id028[1]+\r\n\
\"&n_GPIO_val_029=\" + active_id029[0]+active_id029[1]+\r\n\
\"&n_GPIO_val_030=\" + active_id030[0]+active_id030[1]+\r\n\
\"&n_GPIO_val_031=\" + active_id031[0]+active_id031[1]+\r\n\
\"&n_GPIO_val_032=\" + active_id032[0]+active_id032[1]+\r\n\
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
if(nr_pwm==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==25) document.getElementById(\"tu_pk025\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==26) document.getElementById(\"tu_pk026\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==27) document.getElementById(\"tu_pk027\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==29) document.getElementById(\"tu_pk029\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==30) document.getElementById(\"tu_pk030\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==31) document.getElementById(\"tu_pk031\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==32) document.getElementById(\"tu_pk032\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\r\n\
}\r\n\
}\r\n\
}\r\n\
if(nr_pwm==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\r\n\
if(nr_pwm==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\r\n\
if(nr_pwm==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\r\n\
if(nr_pwm==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\r\n\
if(nr_pwm==25) xmlhttp.open(\"GET\",\"/mobile/pk008\", true);\r\n\
if(nr_pwm==26) xmlhttp.open(\"GET\",\"/mobile/pk009\", true);\r\n\
if(nr_pwm==27) xmlhttp.open(\"GET\",\"/mobile/pk010\", true);\r\n\
if(nr_pwm==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\r\n\
if(nr_pwm==29) xmlhttp.open(\"GET\",\"/mobile/pk002\", true);\r\n\
if(nr_pwm==30) xmlhttp.open(\"GET\",\"/mobile/pk003\", true);\r\n\
if(nr_pwm==31) xmlhttp.open(\"GET\",\"/mobile/pk004\", true);\r\n\
if(nr_pwm==32) xmlhttp.open(\"GET\",\"/mobile/pk005\", true);\r\n\
if(nr_pwm==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\r\n\
xmlhttp.send();\r\n\
}\r\n\
function xpk021(){if(wst==0){wst=1;now=0;nr_pwm=21;loadXMLDoc_PK(); }}\r\n\
function xpk022(){if(wst==0){wst=1;now=0;nr_pwm=22;loadXMLDoc_PK(); }}\r\n\
function xpk023(){if(wst==0){wst=1;now=0;nr_pwm=23;loadXMLDoc_PK(); }}\r\n\
function xpk024(){if(wst==0){wst=1;now=0;nr_pwm=24;loadXMLDoc_PK(); }}\r\n\
function xpk025(){if(wst==0){wst=1;now=0;nr_pwm=25;loadXMLDoc_PK(); }}\r\n\
function xpk026(){if(wst==0){wst=1;now=0;nr_pwm=26;loadXMLDoc_PK(); }}\r\n\
function xpk027(){if(wst==0){wst=1;now=0;nr_pwm=27;loadXMLDoc_PK(); }}\r\n\
function xpk028(){if(wst==0){wst=1;now=0;nr_pwm=28;loadXMLDoc_PK(); }}\r\n\
function xpk029(){if(wst==0){wst=1;now=0;nr_pwm=29;loadXMLDoc_PK(); }}\r\n\
function xpk030(){if(wst==0){wst=1;now=0;nr_pwm=30;loadXMLDoc_PK(); }}\r\n\
function xpk031(){if(wst==0){wst=1;now=0;nr_pwm=31;loadXMLDoc_PK(); }}\r\n\
function xpk032(){if(wst==0){wst=1;now=0;nr_pwm=32;loadXMLDoc_PK(); }}\r\n\
function xpk033(){if(wst==0){wst=1;now=0;nr_pwm=33;loadXMLDoc_PK(); }}\r\n\
function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){if(wst==0){wst=1;now=0;nr_pwm=1; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){if(wst==0){wst=1;now=0;nr_pwm=2; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){if(wst==0){wst=1;now=0;nr_pwm=3; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
var myVar = setInterval(loadXMLDoc, 500      );\r\n\
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
  var tyt=[];   tyt[0]=\"<li><a href='main'>Panel odczytowy</a></li><li><a href='3main'>Konfiguracja portow</a></li><li><a href='lan'>Ustawienia sieci LAN</a></li><li><a href='wifi'>Ustawienia sieci WiFi</a></li><li><a href='gsm'>Ustawienia sieci GSM</a></li><li><a href='czas'>Ustawienia czasu</a></li><li><a href='maskisms'>Numery telefonow</a></li><li><a href='maskimail'>Skrzynki e-mail</a></li><li><a href='logic'>Operacje logiczne</a></li><li><a href='rej'>Rejetrator</a></li><li><a href='Rot'>Rotacja</a></li>\";\r\n\
  document.getElementById(\"tu_menu\").innerHTML=tyt;\r\n\
  </script>\r\n\
\r\n\
  \r\n\
</html>";

const char HttpMainReadPanel2[]="<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\"\"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\r\n\
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
var myVar = setInterval(loadXMLDoc, 500       );\r\n\
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



const char HttpMainSettings[]="<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\"\"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">\r\n\
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
<form method=\"get\" action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 \r\n\
 <div class=\"corner-radius4\" style=\"margin-left: 0px; width: 340px; height:   1933px;   \">\r\n\
 <div class=\"corner-radius3\" style=\"margin-left: 10px; width: 310px; height:  1903px;   \">\r\n\
 <table><tr><td width=\"30px\"></td><td><font class=\"css-font-style\" style=\"font-family: Courier\"><b>Nazwa Portu Nr:</b></font></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 1.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa1_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 1\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 2.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa2_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 2\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 3.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa3_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 3\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 4.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa4_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 4\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 5.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa5_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 5\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 6.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa6_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 6\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 7.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa7_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 7\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 8.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa8_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 8\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">&nbsp; 9.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa9_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 9\"               ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">10.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa10_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 10\"              ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">11.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa11_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 11\"              ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">12.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa12_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 12\"              ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">13.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa13_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 13\"              ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">14.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa14_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 14\"              ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">15.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa15_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 15     0-10V\"    ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">16.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa16_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 16     0-10V\"    ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">17.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa17_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 17     E1\"       ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">18.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa18_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 18     E2\"       ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">19.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa19_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 19     E3\"       ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">20.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa20_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 20     E4\"       ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">21.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa21_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 21     PW1\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">22.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa22_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 22     PW2\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">23.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa23_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 23     PW3\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">24.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa24_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 24     OC1\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">25.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa25_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 25     OC2\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">26.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa26_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 26     OC3\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">27.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa27_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 27     OC4\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">28.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa28_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 28     PK1\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">29.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa29_001\" size=\"30\" maxlength=\"30\" value=\"Moj Przekaznik\"                 ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">30.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa30_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 30     PK3\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">31.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa31_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 31     PK4\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">32.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa32_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 32     PK5\"      ></td></tr></table>\r\n\
<table><tr><td><font class=\"css_list\">33.</font><input type=\"text\" class=\"textbox\" style=\"width:240px;\" name=\"n_s_Port_nazwa33_001\" size=\"30\" maxlength=\"30\" value=\"Nazwa portu nr 33     PK6\"      ></td></tr></table>\r\n\
<table><tr><td width=\"80px\"></td><td><button type=\"submit\" name=\"n_109_zapisz\" class=\"css_button\" style=\"padding: 2px 17px;\" >Zapisz Nazwy</button></td></tr></table>\r\n\
</div></div>\r\n\
</form>\r\n\
<div class=\"corner-radius4\" style=\"margin-left: 0px; width: 340px; height:   1933px;   \">\r\n\
<div class=\"corner-radius3\" style=\"margin-left: 10px; width: 310px; height:  1903px;   \">\r\n\
<table><tr><td><font class=\"css-font-style\" style=\"font-family: Courier\"><b>Tryb Pracy Portu nr:</b></font><br></td></tr></table>\r\n\
\r\n\
<table><tr><td><form name=\"wybor001\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_001\" style=\"width: 290px\">\r\n\
<option value=\"02p001\" selected >&nbsp; 1 &nbsp;Temperatura</option>\r\n\
<option value=\"03p001\"          >&nbsp; 1 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p001\"          >&nbsp; 1 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p001\"          >&nbsp; 1 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p001\"          >&nbsp; 1 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p001\"          >&nbsp; 1 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor002\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_002\" style=\"width: 290px\">\r\n\
<option value=\"02p002\" selected >&nbsp; 2 &nbsp;Temperatura</option>\r\n\
<option value=\"03p002\"          >&nbsp; 2 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p002\"          >&nbsp; 2 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p002\"          >&nbsp; 2 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p002\"          >&nbsp; 2 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p002\"          >&nbsp; 2 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor003\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_003\" style=\"width: 290px\">\r\n\
<option value=\"02p003\"          >&nbsp; 3 &nbsp;Temperatura</option>\r\n\
<option value=\"03p003\" selected >&nbsp; 3 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p003\"          >&nbsp; 3 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p003\"          >&nbsp; 3 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p003\"          >&nbsp; 3 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p003\"          >&nbsp; 3 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor004\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_004\" style=\"width: 290px\">\r\n\
<option value=\"02p004\"          >&nbsp; 4 &nbsp;Temperatura</option>\r\n\
<option value=\"03p004\" selected >&nbsp; 4 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p004\"          >&nbsp; 4 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p004\"          >&nbsp; 4 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p004\"          >&nbsp; 4 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p004\"          >&nbsp; 4 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor005\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_005\" style=\"width: 290px\">\r\n\
<option value=\"02p005\"          >&nbsp; 5 &nbsp;Temperatura</option>\r\n\
<option value=\"03p005\"          >&nbsp; 5 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p005\" selected >&nbsp; 5 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p005\"          >&nbsp; 5 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p005\"          >&nbsp; 5 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p005\"          >&nbsp; 5 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor006\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_006\" style=\"width: 290px\">\r\n\
<option value=\"02p006\"          >&nbsp; 6 &nbsp;Temperatura</option>\r\n\
<option value=\"03p006\"          >&nbsp; 6 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p006\" selected >&nbsp; 6 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p006\"          >&nbsp; 6 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p006\"          >&nbsp; 6 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p006\"          >&nbsp; 6 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor007\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_007\" style=\"width: 290px\">\r\n\
<option value=\"02p007\"          >&nbsp; 7 &nbsp;Temperatura</option>\r\n\
<option value=\"03p007\"          >&nbsp; 7 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p007\"          >&nbsp; 7 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p007\" selected >&nbsp; 7 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p007\"          >&nbsp; 7 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p007\"          >&nbsp; 7 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor008\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_008\" style=\"width: 290px\">\r\n\
<option value=\"02p008\"          >&nbsp; 8 &nbsp;Temperatura</option>\r\n\
<option value=\"03p008\"          >&nbsp; 8 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p008\"          >&nbsp; 8 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p008\" selected >&nbsp; 8 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p008\"          >&nbsp; 8 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p008\"          >&nbsp; 8 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor009\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_009\" style=\"width: 290px\">\r\n\
<option value=\"02p009\"          >&nbsp; 9 &nbsp;Temperatura</option>\r\n\
<option value=\"03p009\"          >&nbsp; 9 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p009\"          >&nbsp; 9 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p009\"          >&nbsp; 9 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p009\" selected >&nbsp; 9 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p009\"          >&nbsp; 9 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor010\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_010\" style=\"width: 290px\">\r\n\
<option value=\"02p010\"          >10 &nbsp;Temperatura</option>\r\n\
<option value=\"03p010\"          >10 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p010\"          >10 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p010\"          >10 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p010\" selected >10 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p010\"          >10 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor011\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_011\" style=\"width: 290px\">\r\n\
<option value=\"02p011\"          >11 &nbsp;Temperatura</option>\r\n\
<option value=\"03p011\"          >11 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p011\"          >11 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p011\"          >11 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p011\"          >11 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p011\" selected >11 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor012\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_012\" style=\"width: 290px\">\r\n\
<option value=\"02p012\"          >12 &nbsp;Temperatura</option>\r\n\
<option value=\"03p012\"          >12 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p012\"          >12 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p012\"          >12 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p012\"          >12 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p012\" selected >12 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor013\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_013\" style=\"width: 290px\">\r\n\
<option value=\"02p013\"          >13 &nbsp;Temperatura</option>\r\n\
<option value=\"03p013\"          >13 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p013\"          >13 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p013\"          >13 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p013\"          >13 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p013\" selected >13 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor014\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_014\" style=\"width: 290px\">\r\n\
<option value=\"02p014\" selected >14 &nbsp;Temperatura</option>\r\n\
<option value=\"03p014\"          >14 &nbsp;Czujnik Pt1000</option>\r\n\
<option value=\"09p014\"          >14 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p014\"          >14 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"13p014\"          >14 &nbsp;Wejscie EOL </option>\r\n\
<option value=\"06p014\"          >14 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor015\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_015\" style=\"width: 290px\">\r\n\
<option value=\"05p015\" selected >15 &nbsp;Pomiar Napiecia 0-10V</option>\r\n\
<option value=\"06p015\"          >15 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor016\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_016\" style=\"width: 290px\">\r\n\
<option value=\"05p016\" selected >16 &nbsp;Pomiar Napiecia 0-10V</option>\r\n\
<option value=\"06p016\"          >16 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor017\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_017\" style=\"width: 290px\">\r\n\
<option value=\"02p017\"          >17 &nbsp;Temperatura</option>\r\n\
<option value=\"09p017\"          >17 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p017\"          >17 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"07p017\" selected >17 &nbsp;Licznik impulsow </option>\r\n\
<option value=\"06p017\"          >17 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor018\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_018\" style=\"width: 290px\">\r\n\
<option value=\"02p018\"          >18 &nbsp;Temperatura</option>\r\n\
<option value=\"09p018\"          >18 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p018\"          >18 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"07p018\" selected >18 &nbsp;Licznik impulsow </option>\r\n\
<option value=\"06p018\"          >18 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor019\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_019\" style=\"width: 290px\">\r\n\
<option value=\"02p019\" selected >19 &nbsp;Temperatura</option>\r\n\
<option value=\"09p019\"          >19 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p019\"          >19 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"07p019\"          >19 &nbsp;Licznik impulsow </option>\r\n\
<option value=\"06p019\"          >19 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor020\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_020\" style=\"width: 290px\">\r\n\
<option value=\"02p020\"          >20 &nbsp;Temperatura</option>\r\n\
<option value=\"09p020\" selected >20 &nbsp;Temperatura Wilgotnosc</option>\r\n\
<option value=\"04p020\"          >20 &nbsp;Czujniki zwar-rozwar</option>\r\n\
<option value=\"07p020\"          >20 &nbsp;Licznik impulsow </option>\r\n\
<option value=\"06p020\"          >20 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor021\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_021\" style=\"width: 290px\">\r\n\
<option value=\"00p021\" selected >21 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p021\"          >21 &nbsp;Timer</option>\r\n\
<option value=\"11p021\"          >21 &nbsp;Termostat</option>\r\n\
<option value=\"01p021\"          >21 &nbsp;Sterowanie PWM</option>\r\n\
<option value=\"08p021\"          >21 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p021\"          >21 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor022\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_022\" style=\"width: 290px\">\r\n\
<option value=\"00p022\" selected >22 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p022\"          >22 &nbsp;Timer</option>\r\n\
<option value=\"11p022\"          >22 &nbsp;Termostat</option>\r\n\
<option value=\"01p022\"          >22 &nbsp;Sterowanie PWM</option>\r\n\
<option value=\"08p022\"          >22 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p022\"          >22 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor023\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_023\" style=\"width: 290px\">\r\n\
<option value=\"00p023\"          >23 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p023\" selected >23 &nbsp;Timer</option>\r\n\
<option value=\"11p023\"          >23 &nbsp;Termostat</option>\r\n\
<option value=\"01p023\"          >23 &nbsp;Sterowanie PWM</option>\r\n\
<option value=\"08p023\"          >23 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p023\"          >23 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor024\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_024\" style=\"width: 290px\">\r\n\
<option value=\"00p024\"          >24 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p024\" selected >24 &nbsp;Timer</option>\r\n\
<option value=\"11p024\"          >24 &nbsp;Termostat</option>\r\n\
<option value=\"08p024\"          >24 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p024\"          >24 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor025\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_025\" style=\"width: 290px\">\r\n\
<option value=\"00p025\"          >25 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p025\"          >25 &nbsp;Timer</option>\r\n\
<option value=\"11p025\" selected >25 &nbsp;Termostat</option>\r\n\
<option value=\"08p025\"          >25 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p025\"          >25 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor026\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_026\" style=\"width: 290px\">\r\n\
<option value=\"00p026\"          >26 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p026\"          >26 &nbsp;Timer</option>\r\n\
<option value=\"11p026\" selected >26 &nbsp;Termostat</option>\r\n\
<option value=\"08p026\"          >26 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p026\"          >26 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor027\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_027\" style=\"width: 290px\">\r\n\
<option value=\"00p027\"          >27 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p027\"          >27 &nbsp;Timer</option>\r\n\
<option value=\"11p027\"          >27 &nbsp;Termostat</option>\r\n\
<option value=\"08p027\" selected >27 &nbsp;Sterowanie IR </option>\r\n\
<option value=\"06p027\"          >27 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor028\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_028\" style=\"width: 290px\">\r\n\
<option value=\"00p028\" selected >28 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p028\"          >28 &nbsp;Timer</option>\r\n\
<option value=\"11p028\"          >28 &nbsp;Termostat</option>\r\n\
<option value=\"06p028\"          >28 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor029\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_029\" style=\"width: 290px\">\r\n\
<option value=\"00p029\"          >29 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p029\" selected >29 &nbsp;Timer</option>\r\n\
<option value=\"11p029\"          >29 &nbsp;Termostat</option>\r\n\
<option value=\"06p029\"          >29 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor030\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_030\" style=\"width: 290px\">\r\n\
<option value=\"00p030\"          >30 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p030\"          >30 &nbsp;Timer</option>\r\n\
<option value=\"11p030\" selected >30 &nbsp;Termostat</option>\r\n\
<option value=\"06p030\"          >30 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor031\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_031\" style=\"width: 290px\">\r\n\
<option value=\"00p031\"          >31 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p031\"          >31 &nbsp;Timer</option>\r\n\
<option value=\"11p031\"          >31 &nbsp;Termostat</option>\r\n\
<option value=\"06p031\" selected >31 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor032\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_032\" style=\"width: 290px\">\r\n\
<option value=\"00p032\"          >32 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p032\"          >32 &nbsp;Timer</option>\r\n\
<option value=\"11p032\"          >32 &nbsp;Termostat</option>\r\n\
<option value=\"06p032\" selected >32 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td><form name=\"wybor033\" method=\"get\"  action=\"lpc.cgi/A117\" data-ajax=\"false\">\r\n\
 <div id=\"mainselection\">\r\n\
 <select name=\"n_GPIO_val_033\" style=\"width: 290px\">\r\n\
<option value=\"00p033\" selected >33 &nbsp;Przekaznik</option>\r\n\
<option value=\"10p033\"          >33 &nbsp;Timer</option>\r\n\
<option value=\"11p033\"          >33 &nbsp;Termostat</option>\r\n\
<option value=\"06p033\"          >33 &nbsp; -- -- -- </option>\r\n\
 </select></div>\r\n\
 </form></td></tr></table>\r\n\
 \r\n\
<table><tr><td width=\"80px\"></td><td><A HREF=\"javascript:zaladuj()\" class=\"css_button\" style=\"padding: 2px 17px;\">Zapisz Tryb</a></td></tr></table>\r\n\
</div></div>\r\n\
<div class=\"corner-radius4\" style=\"margin-left: 0px; width: 370px; height:   1933px;   \">\r\n\
<div class=\"corner-radius3\" style=\"margin-left: 10px; width: 340px; height:  1903px;   \">\r\n\
<table><tr><td><font size=\"4\" color=\"73B10E\"><b>Ustaw</b></font></td><td width=\"50px\"></td><td><font size=\"4\" color=\"f0f0f0\"><b>Odczyty</b></font></td></tr></table>\r\n\
\r\n\
<div id=\"00p001\" style=\"display: none;\"></div>\r\n\
<div id=\"02p001\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp001' class=\"shiny-button\">&nbsp;&nbsp;1</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp001a\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora001a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p001\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp001' class=\"shiny-button\">&nbsp;&nbsp;1</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp001b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp001c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora001b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p001\" style=\"display: none;\"></div>\r\n\
<div id=\"11p001\" style=\"display: none;\"></div>\r\n\
<div id=\"08p001\" style=\"display: none;\"></div>\r\n\
<div id=\"04p001\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we001' class=\"shiny-button\">&nbsp;&nbsp;1</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we001\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora001we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p001\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol001' class=\"shiny-button\">&nbsp;&nbsp;1</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol001\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora001eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p001\" style=\"display: none;\"></div>\r\n\
<div id=\"05p001\" style=\"display: none;\"></div>\r\n\
<div id=\"03p001\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp001' class=\"shiny-button\">&nbsp;&nbsp;1</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt001\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p001\" style=\"display: none;\"></div>\r\n\
<div id=\"06p001\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p002\" style=\"display: none;\"></div>\r\n\
<div id=\"02p002\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp002' class=\"shiny-button\">&nbsp;&nbsp;2</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp002a\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora002a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p002\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp002' class=\"shiny-button\">&nbsp;&nbsp;2</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp002b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp002c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora002b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p002\" style=\"display: none;\"></div>\r\n\
<div id=\"11p002\" style=\"display: none;\"></div>\r\n\
<div id=\"08p002\" style=\"display: none;\"></div>\r\n\
<div id=\"04p002\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we002' class=\"shiny-button\">&nbsp;&nbsp;2</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we002\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora002we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p002\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol002' class=\"shiny-button\">&nbsp;&nbsp;2</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol002\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora002eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p002\" style=\"display: none;\"></div>\r\n\
<div id=\"05p002\" style=\"display: none;\"></div>\r\n\
<div id=\"03p002\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp002' class=\"shiny-button\">&nbsp;&nbsp;2</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt002\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p002\" style=\"display: none;\"></div>\r\n\
<div id=\"06p002\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p003\" style=\"display: none;\"></div>\r\n\
<div id=\"02p003\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp003' class=\"shiny-button\">&nbsp;&nbsp;3</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp003a\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora003a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p003\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp003' class=\"shiny-button\">&nbsp;&nbsp;3</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp003b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp003c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora003b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p003\" style=\"display: none;\"></div>\r\n\
<div id=\"11p003\" style=\"display: none;\"></div>\r\n\
<div id=\"08p003\" style=\"display: none;\"></div>\r\n\
<div id=\"04p003\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we003' class=\"shiny-button\">&nbsp;&nbsp;3</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we003\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora003we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p003\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol003' class=\"shiny-button\">&nbsp;&nbsp;3</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol003\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora003eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p003\" style=\"display: none;\"></div>\r\n\
<div id=\"05p003\" style=\"display: none;\"></div>\r\n\
<div id=\"03p003\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp003' class=\"shiny-button\">&nbsp;&nbsp;3</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt003\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p003\" style=\"display: none;\"></div>\r\n\
<div id=\"06p003\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p004\" style=\"display: none;\"></div>\r\n\
<div id=\"02p004\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp004' class=\"shiny-button\">&nbsp;&nbsp;4</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp004a\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora004a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p004\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp004' class=\"shiny-button\">&nbsp;&nbsp;4</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp004b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp004c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora004b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p004\" style=\"display: none;\"></div>\r\n\
<div id=\"11p004\" style=\"display: none;\"></div>\r\n\
<div id=\"08p004\" style=\"display: none;\"></div>\r\n\
<div id=\"04p004\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we004' class=\"shiny-button\">&nbsp;&nbsp;4</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we004\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora004we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p004\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol004' class=\"shiny-button\">&nbsp;&nbsp;4</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol004\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora004eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p004\" style=\"display: none;\"></div>\r\n\
<div id=\"05p004\" style=\"display: none;\"></div>\r\n\
<div id=\"03p004\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp004' class=\"shiny-button\">&nbsp;&nbsp;4</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt004\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p004\" style=\"display: none;\"></div>\r\n\
<div id=\"06p004\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p005\" style=\"display: none;\"></div>\r\n\
<div id=\"02p005\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp005' class=\"shiny-button\">&nbsp;&nbsp;5</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp005a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora005a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p005\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp005' class=\"shiny-button\">&nbsp;&nbsp;5</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp005b\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp005c\"><font color='#000'>  -- %</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora005b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p005\" style=\"display: none;\"></div>\r\n\
<div id=\"11p005\" style=\"display: none;\"></div>\r\n\
<div id=\"08p005\" style=\"display: none;\"></div>\r\n\
<div id=\"04p005\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we005' class=\"shiny-button\">&nbsp;&nbsp;5</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we005\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora005we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p005\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol005' class=\"shiny-button\">&nbsp;&nbsp;5</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol005\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora005eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p005\" style=\"display: none;\"></div>\r\n\
<div id=\"05p005\" style=\"display: none;\"></div>\r\n\
<div id=\"03p005\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp005' class=\"shiny-button\">&nbsp;&nbsp;5</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt005\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p005\" style=\"display: none;\"></div>\r\n\
<div id=\"06p005\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p006\" style=\"display: none;\"></div>\r\n\
<div id=\"02p006\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp006' class=\"shiny-button\">&nbsp;&nbsp;6</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp006a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora006a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p006\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp006' class=\"shiny-button\">&nbsp;&nbsp;6</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp006b\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp006c\"><font color='#000'>  -- %</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora006b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p006\" style=\"display: none;\"></div>\r\n\
<div id=\"11p006\" style=\"display: none;\"></div>\r\n\
<div id=\"08p006\" style=\"display: none;\"></div>\r\n\
<div id=\"04p006\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we006' class=\"shiny-button\">&nbsp;&nbsp;6</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we006\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora006we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p006\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol006' class=\"shiny-button\">&nbsp;&nbsp;6</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol006\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora006eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p006\" style=\"display: none;\"></div>\r\n\
<div id=\"05p006\" style=\"display: none;\"></div>\r\n\
<div id=\"03p006\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp006' class=\"shiny-button\">&nbsp;&nbsp;6</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt006\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p006\" style=\"display: none;\"></div>\r\n\
<div id=\"06p006\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p007\" style=\"display: none;\"></div>\r\n\
<div id=\"02p007\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp007' class=\"shiny-button\">&nbsp;&nbsp;7</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp007a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora007a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p007\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp007' class=\"shiny-button\">&nbsp;&nbsp;7</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp007b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp007c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora007b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p007\" style=\"display: none;\"></div>\r\n\
<div id=\"11p007\" style=\"display: none;\"></div>\r\n\
<div id=\"08p007\" style=\"display: none;\"></div>\r\n\
<div id=\"04p007\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we007' class=\"shiny-button\">&nbsp;&nbsp;7</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we007\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora007we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p007\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol007' class=\"shiny-button\">&nbsp;&nbsp;7</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol007\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora007eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p007\" style=\"display: none;\"></div>\r\n\
<div id=\"05p007\" style=\"display: none;\"></div>\r\n\
<div id=\"03p007\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp007' class=\"shiny-button\">&nbsp;&nbsp;7</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt007\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p007\" style=\"display: none;\"></div>\r\n\
<div id=\"06p007\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p008\" style=\"display: none;\"></div>\r\n\
<div id=\"02p008\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp008' class=\"shiny-button\">&nbsp;&nbsp;8</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp008a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora008a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p008\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp008' class=\"shiny-button\">&nbsp;&nbsp;8</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp008b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp008c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora008b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p008\" style=\"display: none;\"></div>\r\n\
<div id=\"11p008\" style=\"display: none;\"></div>\r\n\
<div id=\"08p008\" style=\"display: none;\"></div>\r\n\
<div id=\"04p008\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we008' class=\"shiny-button\">&nbsp;&nbsp;8</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we008\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora008we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p008\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol008' class=\"shiny-button\">&nbsp;&nbsp;8</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol008\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora008eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p008\" style=\"display: none;\"></div>\r\n\
<div id=\"05p008\" style=\"display: none;\"></div>\r\n\
<div id=\"03p008\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp008' class=\"shiny-button\">&nbsp;&nbsp;8</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt008\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p008\" style=\"display: none;\"></div>\r\n\
<div id=\"06p008\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p009\" style=\"display: none;\"></div>\r\n\
<div id=\"02p009\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp009' class=\"shiny-button\">&nbsp;&nbsp;9</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp009a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora009a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p009\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp009' class=\"shiny-button\">&nbsp;&nbsp;9</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp009b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp009c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora009b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p009\" style=\"display: none;\"></div>\r\n\
<div id=\"11p009\" style=\"display: none;\"></div>\r\n\
<div id=\"08p009\" style=\"display: none;\"></div>\r\n\
<div id=\"04p009\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we009' class=\"shiny-button\">&nbsp;&nbsp;9</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we009\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora009we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p009\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol009' class=\"shiny-button\">&nbsp;&nbsp;9</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol009\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora009eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p009\" style=\"display: none;\"></div>\r\n\
<div id=\"05p009\" style=\"display: none;\"></div>\r\n\
<div id=\"03p009\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp009' class=\"shiny-button\">&nbsp;&nbsp;9</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt009\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p009\" style=\"display: none;\"></div>\r\n\
<div id=\"06p009\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p010\" style=\"display: none;\"></div>\r\n\
<div id=\"02p010\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp010' class=\"shiny-button\">10</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp010a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora010a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p010\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp010' class=\"shiny-button\">10</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp010b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp010c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora010b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p010\" style=\"display: none;\"></div>\r\n\
<div id=\"11p010\" style=\"display: none;\"></div>\r\n\
<div id=\"08p010\" style=\"display: none;\"></div>\r\n\
<div id=\"04p010\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we010' class=\"shiny-button\">10</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we010\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora010we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p010\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol010' class=\"shiny-button\">10</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol010\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora010eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p010\" style=\"display: none;\"></div>\r\n\
<div id=\"05p010\" style=\"display: none;\"></div>\r\n\
<div id=\"03p010\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp010' class=\"shiny-button\">10</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt010\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p010\" style=\"display: none;\"></div>\r\n\
<div id=\"06p010\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p011\" style=\"display: none;\"></div>\r\n\
<div id=\"02p011\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp011' class=\"shiny-button\">11</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp011a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora011a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p011\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp011' class=\"shiny-button\">11</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp011b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp011c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora011b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p011\" style=\"display: none;\"></div>\r\n\
<div id=\"11p011\" style=\"display: none;\"></div>\r\n\
<div id=\"08p011\" style=\"display: none;\"></div>\r\n\
<div id=\"04p011\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we011' class=\"shiny-button\">11</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we011\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora011we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p011\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol011' class=\"shiny-button\">11</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol011\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora011eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p011\" style=\"display: none;\"></div>\r\n\
<div id=\"05p011\" style=\"display: none;\"></div>\r\n\
<div id=\"03p011\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp011' class=\"shiny-button\">11</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt011\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p011\" style=\"display: none;\"></div>\r\n\
<div id=\"06p011\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p012\" style=\"display: none;\"></div>\r\n\
<div id=\"02p012\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp012' class=\"shiny-button\">12</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp012a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora012a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p012\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp012' class=\"shiny-button\">12</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp012b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp012c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora012b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p012\" style=\"display: none;\"></div>\r\n\
<div id=\"11p012\" style=\"display: none;\"></div>\r\n\
<div id=\"08p012\" style=\"display: none;\"></div>\r\n\
<div id=\"04p012\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we012' class=\"shiny-button\">12</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we012\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora012we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p012\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol012' class=\"shiny-button\">12</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol012\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora012eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p012\" style=\"display: none;\"></div>\r\n\
<div id=\"05p012\" style=\"display: none;\"></div>\r\n\
<div id=\"03p012\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp012' class=\"shiny-button\">12</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt012\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p012\" style=\"display: none;\"></div>\r\n\
<div id=\"06p012\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p013\" style=\"display: none;\"></div>\r\n\
<div id=\"02p013\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp013' class=\"shiny-button\">13</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp013a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora013a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p013\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp013' class=\"shiny-button\">13</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp013b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp013c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora013b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p013\" style=\"display: none;\"></div>\r\n\
<div id=\"11p013\" style=\"display: none;\"></div>\r\n\
<div id=\"08p013\" style=\"display: none;\"></div>\r\n\
<div id=\"04p013\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we013' class=\"shiny-button\">13</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we013\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora013we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p013\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol013' class=\"shiny-button\">13</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol013\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora013eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p013\" style=\"display: none;\"></div>\r\n\
<div id=\"05p013\" style=\"display: none;\"></div>\r\n\
<div id=\"03p013\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp013' class=\"shiny-button\">13</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt013\"> --.-</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p013\" style=\"display: none;\"></div>\r\n\
<div id=\"06p013\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p014\" style=\"display: none;\"></div>\r\n\
<div id=\"02p014\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp014' class=\"shiny-button\">14</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp014a\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora014a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p014\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp014' class=\"shiny-button\">14</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp014b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp014c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora014b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p014\" style=\"display: none;\"></div>\r\n\
<div id=\"11p014\" style=\"display: none;\"></div>\r\n\
<div id=\"08p014\" style=\"display: none;\"></div>\r\n\
<div id=\"04p014\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we014' class=\"shiny-button\">14</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we014\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora014we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p014\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='eol014' class=\"shiny-button\">14</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_eol014\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora014eol\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"01p014\" style=\"display: none;\"></div>\r\n\
<div id=\"05p014\" style=\"display: none;\"></div>\r\n\
<div id=\"03p014\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp014' class=\"shiny-button\">14</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_ptt014\"> --- °C</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"07p014\" style=\"display: none;\"></div>\r\n\
<div id=\"06p014\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p015\" style=\"display: none;\"></div>\r\n\
<div id=\"02p015\" style=\"display: none;\"></div>\r\n\
<div id=\"09p015\" style=\"display: none;\"></div>\r\n\
<div id=\"10p015\" style=\"display: none;\"></div>\r\n\
<div id=\"11p015\" style=\"display: none;\"></div>\r\n\
<div id=\"08p015\" style=\"display: none;\"></div>\r\n\
<div id=\"04p015\" style=\"display: none;\"></div>\r\n\
<div id=\"13p015\" style=\"display: none;\"></div>\r\n\
<div id=\"01p015\" style=\"display: none;\"></div>\r\n\
<div id=\"05p015\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='nap001' class=\"shiny-button\">15</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_adc015\">     7.5   V</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"03p015\" style=\"display: none;\"></div>\r\n\
<div id=\"07p015\" style=\"display: none;\"></div>\r\n\
<div id=\"06p015\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p016\" style=\"display: none;\"></div>\r\n\
<div id=\"02p016\" style=\"display: none;\"></div>\r\n\
<div id=\"09p016\" style=\"display: none;\"></div>\r\n\
<div id=\"10p016\" style=\"display: none;\"></div>\r\n\
<div id=\"11p016\" style=\"display: none;\"></div>\r\n\
<div id=\"08p016\" style=\"display: none;\"></div>\r\n\
<div id=\"04p016\" style=\"display: none;\"></div>\r\n\
<div id=\"13p016\" style=\"display: none;\"></div>\r\n\
<div id=\"01p016\" style=\"display: none;\"></div>\r\n\
<div id=\"05p016\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='nap002' class=\"shiny-button\">16</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_adc016\">     6.7   V</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"03p016\" style=\"display: none;\"></div>\r\n\
<div id=\"07p016\" style=\"display: none;\"></div>\r\n\
<div id=\"06p016\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p017\" style=\"display: none;\"></div>\r\n\
<div id=\"02p017\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp017' class=\"shiny-button\">17</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp017a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora017a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p017\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp017' class=\"shiny-button\">17</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp017b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp017c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora017b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p017\" style=\"display: none;\"></div>\r\n\
<div id=\"11p017\" style=\"display: none;\"></div>\r\n\
<div id=\"08p017\" style=\"display: none;\"></div>\r\n\
<div id=\"04p017\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we015' class=\"shiny-button\">17</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we017\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora017we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p017\" style=\"display: none;\"></div>\r\n\
<div id=\"01p017\" style=\"display: none;\"></div>\r\n\
<div id=\"05p017\" style=\"display: none;\"></div>\r\n\
<div id=\"03p017\" style=\"display: none;\"></div>\r\n\
<div id=\"07p017\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='etr001' class=\"shiny-button\">17</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_imp0_017\">0.000 KWh</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"06p017\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p018\" style=\"display: none;\"></div>\r\n\
<div id=\"02p018\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp018' class=\"shiny-button\">18</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp018a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora018a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p018\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp018' class=\"shiny-button\">18</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp018b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp018c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora018b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p018\" style=\"display: none;\"></div>\r\n\
<div id=\"11p018\" style=\"display: none;\"></div>\r\n\
<div id=\"08p018\" style=\"display: none;\"></div>\r\n\
<div id=\"04p018\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we016' class=\"shiny-button\">18</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we018\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora018we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p018\" style=\"display: none;\"></div>\r\n\
<div id=\"01p018\" style=\"display: none;\"></div>\r\n\
<div id=\"05p018\" style=\"display: none;\"></div>\r\n\
<div id=\"03p018\" style=\"display: none;\"></div>\r\n\
<div id=\"07p018\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='etr002' class=\"shiny-button\">18</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_imp0_018\">0.000 KWh</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"06p018\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p019\" style=\"display: none;\"></div>\r\n\
<div id=\"02p019\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp019' class=\"shiny-button\">19</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp019a\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora019a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p019\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp019' class=\"shiny-button\">19</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp019b\"> --.-</div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp019c\"> -- </div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora019b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p019\" style=\"display: none;\"></div>\r\n\
<div id=\"11p019\" style=\"display: none;\"></div>\r\n\
<div id=\"08p019\" style=\"display: none;\"></div>\r\n\
<div id=\"04p019\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we017' class=\"shiny-button\">19</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we019\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora019we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p019\" style=\"display: none;\"></div>\r\n\
<div id=\"01p019\" style=\"display: none;\"></div>\r\n\
<div id=\"05p019\" style=\"display: none;\"></div>\r\n\
<div id=\"03p019\" style=\"display: none;\"></div>\r\n\
<div id=\"07p019\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='etr003' class=\"shiny-button\">19</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_imp0_019\">0.000 KWh</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"06p019\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p020\" style=\"display: none;\"></div>\r\n\
<div id=\"02p020\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp020' class=\"shiny-button\">20</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp020a\"> --.-</div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora020a\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"09p020\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='temp020' class=\"shiny-button\">20</a></td><td width=\"10px\"></td><td><button class=\"m\"><div id=\"tu_temp020b\"><font color='#000'>   --.- °C</font></div></button></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_temp020c\"><font color='#000'>  -- %</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora020b\"></div></td><td width=\"10px\"></td></tr></table></div>\r\n\
<div id=\"10p020\" style=\"display: none;\"></div>\r\n\
<div id=\"11p020\" style=\"display: none;\"></div>\r\n\
<div id=\"08p020\" style=\"display: none;\"></div>\r\n\
<div id=\"04p020\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='we018' class=\"shiny-button\">20</a></td><td width=\"1px\"></td><td><button class=\"m\"><div id=\"tu_we020\"><font color='#000'>Stan OFF</font></div></button></td><td width=\"1px\"></td><td><div id=\"tu_lora020we\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"13p020\" style=\"display: none;\"></div>\r\n\
<div id=\"01p020\" style=\"display: none;\"></div>\r\n\
<div id=\"05p020\" style=\"display: none;\"></div>\r\n\
<div id=\"03p020\" style=\"display: none;\"></div>\r\n\
<div id=\"07p020\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='etr004' class=\"shiny-button\">20</a></td><td width=\"10px\"></td><td><button class=\"m\"> <div id=\"tu_imp0_020\">0.000 KWh</div></button></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"06p020\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p021\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk011' class=\"shiny-button\">21</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk021(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk021\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora021wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p021\" style=\"display: none;\"></div>\r\n\
<div id=\"09p021\" style=\"display: none;\"></div>\r\n\
<div id=\"10p021\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim011' class=\"shiny-button\">21</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti021\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p021\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term011' class=\"shiny-button\">21</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te021\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p021\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk011' class=\"shiny-button\">21</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk021(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk021\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p021\" style=\"display: none;\"></div>\r\n\
<div id=\"13p021\" style=\"display: none;\"></div>\r\n\
<div id=\"01p021\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='3main' class=\"shiny-button\">21</a></td><td width=\"12px\"></td><td width=\"42px\"><output  class=\"v\" id=\"volume021\">50</output></td><td><input type=\"range\" name=\"n_pwm1\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"50\"   oninput=\"outputUpdate021(value)\" onchange=\"x021(this.value);\"></input></td></tr></table></div>\r\n\
<div id=\"05p021\" style=\"display: none;\"></div>\r\n\
<div id=\"03p021\" style=\"display: none;\"></div>\r\n\
<div id=\"07p021\" style=\"display: none;\"></div>\r\n\
<div id=\"06p021\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p022\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk012' class=\"shiny-button\">22</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk022(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk022\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora022wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p022\" style=\"display: none;\"></div>\r\n\
<div id=\"09p022\" style=\"display: none;\"></div>\r\n\
<div id=\"10p022\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim012' class=\"shiny-button\">22</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti022\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p022\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term012' class=\"shiny-button\">22</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te022\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p022\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk012' class=\"shiny-button\">22</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk022(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk022\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p022\" style=\"display: none;\"></div>\r\n\
<div id=\"13p022\" style=\"display: none;\"></div>\r\n\
<div id=\"01p022\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='3main' class=\"shiny-button\">22</a></td><td width=\"12px\"></td><td width=\"42px\"><output  class=\"v\" id=\"volume022\">18</output></td><td><input type=\"range\" name=\"n_pwm2\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"18\"   oninput=\"outputUpdate022(value)\" onchange=\"x022(this.value);\"></input></td></tr></table></div>\r\n\
<div id=\"05p022\" style=\"display: none;\"></div>\r\n\
<div id=\"03p022\" style=\"display: none;\"></div>\r\n\
<div id=\"07p022\" style=\"display: none;\"></div>\r\n\
<div id=\"06p022\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p023\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk013' class=\"shiny-button\">23</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk023(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk023\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora023wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p023\" style=\"display: none;\"></div>\r\n\
<div id=\"09p023\" style=\"display: none;\"></div>\r\n\
<div id=\"10p023\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim013' class=\"shiny-button\">23</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti023\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p023\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term013' class=\"shiny-button\">23</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te023\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p023\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk013' class=\"shiny-button\">23</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk023(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk023\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p023\" style=\"display: none;\"></div>\r\n\
<div id=\"13p023\" style=\"display: none;\"></div>\r\n\
<div id=\"01p023\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='3main' class=\"shiny-button\">23</a></td><td width=\"12px\"></td><td width=\"42px\"><output  class=\"v\" id=\"volume023\">50</output></td><td><input type=\"range\" name=\"n_pwm3\"  min=\"0\" max=\"100\"  step=\"1\"  value=\"50\"   oninput=\"outputUpdate023(value)\" onchange=\"x023(this.value);\"></input></td></tr></table></div>\r\n\
<div id=\"05p023\" style=\"display: none;\"></div>\r\n\
<div id=\"03p023\" style=\"display: none;\"></div>\r\n\
<div id=\"07p023\" style=\"display: none;\"></div>\r\n\
<div id=\"06p023\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p024\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk007' class=\"shiny-button\">24</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk024(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk024\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora024wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p024\" style=\"display: none;\"></div>\r\n\
<div id=\"09p024\" style=\"display: none;\"></div>\r\n\
<div id=\"10p024\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim007' class=\"shiny-button\">24</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti024\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p024\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term007' class=\"shiny-button\">24</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te024\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p024\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk007' class=\"shiny-button\">24</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk024(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk024\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p024\" style=\"display: none;\"></div>\r\n\
<div id=\"13p024\" style=\"display: none;\"></div>\r\n\
<div id=\"01p024\" style=\"display: none;\"></div>\r\n\
<div id=\"05p024\" style=\"display: none;\"></div>\r\n\
<div id=\"03p024\" style=\"display: none;\"></div>\r\n\
<div id=\"07p024\" style=\"display: none;\"></div>\r\n\
<div id=\"06p024\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p025\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk008' class=\"shiny-button\">25</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk025(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk025\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora025wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p025\" style=\"display: none;\"></div>\r\n\
<div id=\"09p025\" style=\"display: none;\"></div>\r\n\
<div id=\"10p025\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim008' class=\"shiny-button\">25</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti025\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p025\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term008' class=\"shiny-button\">25</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te025\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p025\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk008' class=\"shiny-button\">25</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk025(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk025\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p025\" style=\"display: none;\"></div>\r\n\
<div id=\"13p025\" style=\"display: none;\"></div>\r\n\
<div id=\"01p025\" style=\"display: none;\"></div>\r\n\
<div id=\"05p025\" style=\"display: none;\"></div>\r\n\
<div id=\"03p025\" style=\"display: none;\"></div>\r\n\
<div id=\"07p025\" style=\"display: none;\"></div>\r\n\
<div id=\"06p025\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p026\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk009' class=\"shiny-button\">26</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk026(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk026\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora026wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p026\" style=\"display: none;\"></div>\r\n\
<div id=\"09p026\" style=\"display: none;\"></div>\r\n\
<div id=\"10p026\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim009' class=\"shiny-button\">26</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti026\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p026\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term009' class=\"shiny-button\">26</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te026\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p026\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk009' class=\"shiny-button\">26</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk026(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk026\"><font color=\"#fff\">Stan ON</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p026\" style=\"display: none;\"></div>\r\n\
<div id=\"13p026\" style=\"display: none;\"></div>\r\n\
<div id=\"01p026\" style=\"display: none;\"></div>\r\n\
<div id=\"05p026\" style=\"display: none;\"></div>\r\n\
<div id=\"03p026\" style=\"display: none;\"></div>\r\n\
<div id=\"07p026\" style=\"display: none;\"></div>\r\n\
<div id=\"06p026\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p027\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk010' class=\"shiny-button\">27</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk027(); this.href='javascript:return false;';\" class=\"c\"><div id=\"uu_pk027\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora027wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p027\" style=\"display: none;\"></div>\r\n\
<div id=\"09p027\" style=\"display: none;\"></div>\r\n\
<div id=\"10p027\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim010' class=\"shiny-button\">27</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti027\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p027\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term010' class=\"shiny-button\">27</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te027\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p027\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk010' class=\"shiny-button\">27</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk027(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk027\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"04p027\" style=\"display: none;\"></div>\r\n\
<div id=\"13p027\" style=\"display: none;\"></div>\r\n\
<div id=\"01p027\" style=\"display: none;\"></div>\r\n\
<div id=\"05p027\" style=\"display: none;\"></div>\r\n\
<div id=\"03p027\" style=\"display: none;\"></div>\r\n\
<div id=\"07p027\" style=\"display: none;\"></div>\r\n\
<div id=\"06p027\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p028\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk001' class=\"shiny-button\">28</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk028(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk028\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora028wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p028\" style=\"display: none;\"></div>\r\n\
<div id=\"09p028\" style=\"display: none;\"></div>\r\n\
<div id=\"10p028\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim001' class=\"shiny-button\">28</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti028\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p028\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term001' class=\"shiny-button\">28</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te028\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p028\" style=\"display: none;\"></div>\r\n\
<div id=\"04p028\" style=\"display: none;\"></div>\r\n\
<div id=\"13p028\" style=\"display: none;\"></div>\r\n\
<div id=\"01p028\" style=\"display: none;\"></div>\r\n\
<div id=\"05p028\" style=\"display: none;\"></div>\r\n\
<div id=\"03p028\" style=\"display: none;\"></div>\r\n\
<div id=\"07p028\" style=\"display: none;\"></div>\r\n\
<div id=\"06p028\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p029\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk002' class=\"shiny-button\">29</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk029(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk029\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora029wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p029\" style=\"display: none;\"></div>\r\n\
<div id=\"09p029\" style=\"display: none;\"></div>\r\n\
<div id=\"10p029\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim002' class=\"shiny-button\">29</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti029\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p029\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term002' class=\"shiny-button\">29</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te029\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p029\" style=\"display: none;\"></div>\r\n\
<div id=\"04p029\" style=\"display: none;\"></div>\r\n\
<div id=\"13p029\" style=\"display: none;\"></div>\r\n\
<div id=\"01p029\" style=\"display: none;\"></div>\r\n\
<div id=\"05p029\" style=\"display: none;\"></div>\r\n\
<div id=\"03p029\" style=\"display: none;\"></div>\r\n\
<div id=\"07p029\" style=\"display: none;\"></div>\r\n\
<div id=\"06p029\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p030\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk003' class=\"shiny-button\">30</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk030(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk030\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora030wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p030\" style=\"display: none;\"></div>\r\n\
<div id=\"09p030\" style=\"display: none;\"></div>\r\n\
<div id=\"10p030\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim003' class=\"shiny-button\">30</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti030\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p030\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term003' class=\"shiny-button\">30</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te030\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p030\" style=\"display: none;\"></div>\r\n\
<div id=\"04p030\" style=\"display: none;\"></div>\r\n\
<div id=\"13p030\" style=\"display: none;\"></div>\r\n\
<div id=\"01p030\" style=\"display: none;\"></div>\r\n\
<div id=\"05p030\" style=\"display: none;\"></div>\r\n\
<div id=\"03p030\" style=\"display: none;\"></div>\r\n\
<div id=\"07p030\" style=\"display: none;\"></div>\r\n\
<div id=\"06p030\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p031\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk004' class=\"shiny-button\">31</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk031(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk031\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora031wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p031\" style=\"display: none;\"></div>\r\n\
<div id=\"09p031\" style=\"display: none;\"></div>\r\n\
<div id=\"10p031\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim004' class=\"shiny-button\">31</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti031\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p031\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term004' class=\"shiny-button\">31</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te031\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p031\" style=\"display: none;\"></div>\r\n\
<div id=\"04p031\" style=\"display: none;\"></div>\r\n\
<div id=\"13p031\" style=\"display: none;\"></div>\r\n\
<div id=\"01p031\" style=\"display: none;\"></div>\r\n\
<div id=\"05p031\" style=\"display: none;\"></div>\r\n\
<div id=\"03p031\" style=\"display: none;\"></div>\r\n\
<div id=\"07p031\" style=\"display: none;\"></div>\r\n\
<div id=\"06p031\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p032\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk005' class=\"shiny-button\">32</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk032(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk032\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora032wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p032\" style=\"display: none;\"></div>\r\n\
<div id=\"09p032\" style=\"display: none;\"></div>\r\n\
<div id=\"10p032\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim005' class=\"shiny-button\">32</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti032\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p032\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term005' class=\"shiny-button\">32</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te032\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p032\" style=\"display: none;\"></div>\r\n\
<div id=\"04p032\" style=\"display: none;\"></div>\r\n\
<div id=\"13p032\" style=\"display: none;\"></div>\r\n\
<div id=\"01p032\" style=\"display: none;\"></div>\r\n\
<div id=\"05p032\" style=\"display: none;\"></div>\r\n\
<div id=\"03p032\" style=\"display: none;\"></div>\r\n\
<div id=\"07p032\" style=\"display: none;\"></div>\r\n\
<div id=\"06p032\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
\r\n\
<div id=\"00p033\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='wypk006' class=\"shiny-button\">33</a></td><td width=\"1px\"></td><td><a href=\"#\" rel=\"external\" onClick=\"xpk033(); this.href='javascript:return false;';\" class=\"c\"><div id=\"tu_pk033\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"1px\"></td><td><div id=\"tu_lora033wy\"></div></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"02p033\" style=\"display: none;\"></div>\r\n\
<div id=\"09p033\" style=\"display: none;\"></div>\r\n\
<div id=\"10p033\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='tim006' class=\"shiny-button\">33</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_ti033\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"11p033\" style=\"display: none;\"><table><tr><td width=\"0px\"></td><td width=\"20px\"><a href='term006' class=\"shiny-button\">33</a></td><td width=\"1px\"></td><td><a class=\"c\"><div id=\"tu_te033\"><font color=\"#000\">Stan OFF</font></div></a></td><td width=\"10px\"></td><td></td></tr></table></div>\r\n\
<div id=\"08p033\" style=\"display: none;\"></div>\r\n\
<div id=\"04p033\" style=\"display: none;\"></div>\r\n\
<div id=\"13p033\" style=\"display: none;\"></div>\r\n\
<div id=\"01p033\" style=\"display: none;\"></div>\r\n\
<div id=\"05p033\" style=\"display: none;\"></div>\r\n\
<div id=\"03p033\" style=\"display: none;\"></div>\r\n\
<div id=\"07p033\" style=\"display: none;\"></div>\r\n\
<div id=\"06p033\" style=\"display: none;\"><table><tr><td width=\"50px\"></td><td><font size=\"4\" color=\"white\"  >---&nbsp; Wybierz funkcje&nbsp; ---</font></td></tr></table></div>\r\n\
    </div></div>\r\n\
</div>\r\n\
</body>\r\n\
<script type=\"text/javascript\">\r\n\
window.onload = function(){\r\n\
Laduj();\r\n\
pageScroll();\r\n\
 };\r\n\
var active_id001=\"02p001\";\r\n\
var active_id002=\"02p002\";\r\n\
var active_id003=\"03p003\";\r\n\
var active_id004=\"03p004\";\r\n\
var active_id005=\"09p005\";\r\n\
var active_id006=\"09p006\";\r\n\
var active_id007=\"04p007\";\r\n\
var active_id008=\"04p008\";\r\n\
var active_id009=\"13p009\";\r\n\
var active_id010=\"13p010\";\r\n\
var active_id011=\"06p011\";\r\n\
var active_id012=\"06p012\";\r\n\
var active_id013=\"06p013\";\r\n\
var active_id014=\"02p014\";\r\n\
var active_id015=\"05p015\";\r\n\
var active_id016=\"05p016\";\r\n\
var active_id017=\"07p017\";\r\n\
var active_id018=\"07p018\";\r\n\
var active_id019=\"02p019\";\r\n\
var active_id020=\"09p020\";\r\n\
var active_id021=\"00p021\";\r\n\
var active_id022=\"00p022\";\r\n\
var active_id023=\"10p023\";\r\n\
var active_id024=\"10p024\";\r\n\
var active_id025=\"11p025\";\r\n\
var active_id026=\"11p026\";\r\n\
var active_id027=\"08p027\";\r\n\
var active_id028=\"00p028\";\r\n\
var active_id029=\"10p029\";\r\n\
var active_id030=\"11p030\";\r\n\
var active_id031=\"06p031\";\r\n\
var active_id032=\"06p032\";\r\n\
var active_id033=\"00p033\";\r\n\
function Laduj()\r\n\
{\r\n\
var blok2 = document.getElementById(active_id001);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id002);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id003);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id004);\r\n\
blok2.style.display = \"block\";\r\n\
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
var blok2 = document.getElementById(active_id011);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id012);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id013);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id014);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id015);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id016);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id017);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id018);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id019);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id020);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id021);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id022);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id023);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id024);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id025);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id026);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id027);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id028);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id029);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id030);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id031);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id032);\r\n\
blok2.style.display = \"block\";\r\n\
var blok2 = document.getElementById(active_id033);\r\n\
blok2.style.display = \"block\";\r\n\
\r\n\
document.forms['wybor001'].n_GPIO_val_001.onchange = Zmien001;\r\n\
document.forms['wybor002'].n_GPIO_val_002.onchange = Zmien002;\r\n\
document.forms['wybor003'].n_GPIO_val_003.onchange = Zmien003;\r\n\
document.forms['wybor004'].n_GPIO_val_004.onchange = Zmien004;\r\n\
document.forms['wybor005'].n_GPIO_val_005.onchange = Zmien005;\r\n\
document.forms['wybor006'].n_GPIO_val_006.onchange = Zmien006;\r\n\
document.forms['wybor007'].n_GPIO_val_007.onchange = Zmien007;\r\n\
document.forms['wybor008'].n_GPIO_val_008.onchange = Zmien008;\r\n\
document.forms['wybor009'].n_GPIO_val_009.onchange = Zmien009;\r\n\
document.forms['wybor010'].n_GPIO_val_010.onchange = Zmien010;\r\n\
document.forms['wybor011'].n_GPIO_val_011.onchange = Zmien011;\r\n\
document.forms['wybor012'].n_GPIO_val_012.onchange = Zmien012;\r\n\
document.forms['wybor013'].n_GPIO_val_013.onchange = Zmien013;\r\n\
document.forms['wybor014'].n_GPIO_val_014.onchange = Zmien014;\r\n\
document.forms['wybor015'].n_GPIO_val_015.onchange = Zmien015;\r\n\
document.forms['wybor016'].n_GPIO_val_016.onchange = Zmien016;\r\n\
document.forms['wybor017'].n_GPIO_val_017.onchange = Zmien017;\r\n\
document.forms['wybor018'].n_GPIO_val_018.onchange = Zmien018;\r\n\
document.forms['wybor019'].n_GPIO_val_019.onchange = Zmien019;\r\n\
document.forms['wybor020'].n_GPIO_val_020.onchange = Zmien020;\r\n\
document.forms['wybor021'].n_GPIO_val_021.onchange = Zmien021;\r\n\
document.forms['wybor022'].n_GPIO_val_022.onchange = Zmien022;\r\n\
document.forms['wybor023'].n_GPIO_val_023.onchange = Zmien023;\r\n\
document.forms['wybor024'].n_GPIO_val_024.onchange = Zmien024;\r\n\
document.forms['wybor025'].n_GPIO_val_025.onchange = Zmien025;\r\n\
document.forms['wybor026'].n_GPIO_val_026.onchange = Zmien026;\r\n\
document.forms['wybor027'].n_GPIO_val_027.onchange = Zmien027;\r\n\
document.forms['wybor028'].n_GPIO_val_028.onchange = Zmien028;\r\n\
document.forms['wybor029'].n_GPIO_val_029.onchange = Zmien029;\r\n\
document.forms['wybor030'].n_GPIO_val_030.onchange = Zmien030;\r\n\
document.forms['wybor031'].n_GPIO_val_031.onchange = Zmien031;\r\n\
document.forms['wybor032'].n_GPIO_val_032.onchange = Zmien032;\r\n\
document.forms['wybor033'].n_GPIO_val_033.onchange = Zmien033;\r\n\
}\r\n\
 function Zmien001()\r\n\
 {\r\n\
 if (active_id001 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id001);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id001 = this.value;\r\n\
 }\r\n\
 function Zmien002()\r\n\
 {\r\n\
 if (active_id002 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id002);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id002 = this.value;\r\n\
 }\r\n\
 function Zmien003()\r\n\
 {\r\n\
 if (active_id003 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id003);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id003 = this.value;\r\n\
 }\r\n\
 function Zmien004()\r\n\
 {\r\n\
 if (active_id004 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id004);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id004 = this.value;\r\n\
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
 function Zmien011()\r\n\
 {\r\n\
 if (active_id011 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id011);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id011 = this.value;\r\n\
 }\r\n\
 function Zmien012()\r\n\
 {\r\n\
 if (active_id012 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id012);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id012 = this.value;\r\n\
 }\r\n\
 function Zmien013()\r\n\
 {\r\n\
 if (active_id013 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id013);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id013 = this.value;\r\n\
 }\r\n\
 function Zmien014()\r\n\
 {\r\n\
 if (active_id014 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id014);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id014 = this.value;\r\n\
 }\r\n\
 function Zmien015()\r\n\
 {\r\n\
 if (active_id015 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id015);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id015 = this.value;\r\n\
 }\r\n\
 function Zmien016()\r\n\
 {\r\n\
 if (active_id016 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id016);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id016 = this.value;\r\n\
 }\r\n\
 function Zmien017()\r\n\
 {\r\n\
 if (active_id017 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id017);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id017 = this.value;\r\n\
 }\r\n\
 function Zmien018()\r\n\
 {\r\n\
 if (active_id018 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id018);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id018 = this.value;\r\n\
 }\r\n\
 function Zmien019()\r\n\
 {\r\n\
 if (active_id019 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id019);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id019 = this.value;\r\n\
 }\r\n\
 function Zmien020()\r\n\
 {\r\n\
 if (active_id020 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id020);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id020 = this.value;\r\n\
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
 function Zmien025()\r\n\
 {\r\n\
 if (active_id025 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id025);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id025 = this.value;\r\n\
 }\r\n\
 function Zmien026()\r\n\
 {\r\n\
 if (active_id026 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id026);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id026 = this.value;\r\n\
 }\r\n\
 function Zmien027()\r\n\
 {\r\n\
 if (active_id027 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id027);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id027 = this.value;\r\n\
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
 function Zmien029()\r\n\
 {\r\n\
 if (active_id029 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id029);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id029 = this.value;\r\n\
 }\r\n\
 function Zmien030()\r\n\
 {\r\n\
 if (active_id030 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id030);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id030 = this.value;\r\n\
 }\r\n\
 function Zmien031()\r\n\
 {\r\n\
 if (active_id031 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id031);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id031 = this.value;\r\n\
 }\r\n\
 function Zmien032()\r\n\
 {\r\n\
 if (active_id032 != \"\")\r\n\
 {\r\n\
 var blok2 = document.getElementById(active_id032);\r\n\
 blok2.style.display = \"none\";\r\n\
 }\r\n\
 var blok = document.getElementById(this.value);\r\n\
 blok.style.display = \"block\";\r\n\
 active_id032 = this.value;\r\n\
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
window.location.href=\"lpc.cgi/n_GPIO_val_001=\" + active_id001[0]+active_id001[1] +\r\n\
\"&n_GPIO_val_002=\" + active_id002[0]+active_id002[1]+\r\n\
\"&n_GPIO_val_003=\" + active_id003[0]+active_id003[1]+\r\n\
\"&n_GPIO_val_004=\" + active_id004[0]+active_id004[1]+\r\n\
\"&n_GPIO_val_005=\" + active_id005[0]+active_id005[1]+\r\n\
\"&n_GPIO_val_006=\" + active_id006[0]+active_id006[1]+\r\n\
\"&n_GPIO_val_007=\" + active_id007[0]+active_id007[1]+\r\n\
\"&n_GPIO_val_008=\" + active_id008[0]+active_id008[1]+\r\n\
\"&n_GPIO_val_009=\" + active_id009[0]+active_id009[1]+\r\n\
\"&n_GPIO_val_010=\" + active_id010[0]+active_id010[1]+\r\n\
\"&n_GPIO_val_011=\" + active_id011[0]+active_id011[1]+\r\n\
\"&n_GPIO_val_012=\" + active_id012[0]+active_id012[1]+\r\n\
\"&n_GPIO_val_013=\" + active_id013[0]+active_id013[1]+\r\n\
\"&n_GPIO_val_014=\" + active_id014[0]+active_id014[1]+\r\n\
\"&n_GPIO_val_015=\" + active_id015[0]+active_id015[1]+\r\n\
\"&n_GPIO_val_016=\" + active_id016[0]+active_id016[1]+\r\n\
\"&n_GPIO_val_017=\" + active_id017[0]+active_id017[1]+\r\n\
\"&n_GPIO_val_018=\" + active_id018[0]+active_id018[1]+\r\n\
\"&n_GPIO_val_019=\" + active_id019[0]+active_id019[1]+\r\n\
\"&n_GPIO_val_020=\" + active_id020[0]+active_id020[1]+\r\n\
\"&n_GPIO_val_021=\" + active_id021[0]+active_id021[1]+\r\n\
\"&n_GPIO_val_022=\" + active_id022[0]+active_id022[1]+\r\n\
\"&n_GPIO_val_023=\" + active_id023[0]+active_id023[1]+\r\n\
\"&n_GPIO_val_024=\" + active_id024[0]+active_id024[1]+\r\n\
\"&n_GPIO_val_025=\" + active_id025[0]+active_id025[1]+\r\n\
\"&n_GPIO_val_026=\" + active_id026[0]+active_id026[1]+\r\n\
\"&n_GPIO_val_027=\" + active_id027[0]+active_id027[1]+\r\n\
\"&n_GPIO_val_028=\" + active_id028[0]+active_id028[1]+\r\n\
\"&n_GPIO_val_029=\" + active_id029[0]+active_id029[1]+\r\n\
\"&n_GPIO_val_030=\" + active_id030[0]+active_id030[1]+\r\n\
\"&n_GPIO_val_031=\" + active_id031[0]+active_id031[1]+\r\n\
\"&n_GPIO_val_032=\" + active_id032[0]+active_id032[1]+\r\n\
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
if(nr_pwm==21) document.getElementById(\"tu_pk021\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==22) document.getElementById(\"tu_pk022\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==23) document.getElementById(\"tu_pk023\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==24) document.getElementById(\"tu_pk024\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==25) document.getElementById(\"tu_pk025\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==26) document.getElementById(\"tu_pk026\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==27) document.getElementById(\"tu_pk027\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==28) document.getElementById(\"tu_pk028\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==29) document.getElementById(\"tu_pk029\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==30) document.getElementById(\"tu_pk030\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==31) document.getElementById(\"tu_pk031\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==32) document.getElementById(\"tu_pk032\").innerHTML=xmlhttp.responseText;\r\n\
if(nr_pwm==33) document.getElementById(\"tu_pk033\").innerHTML=xmlhttp.responseText;\r\n\
}\r\n\
}\r\n\
}\r\n\
if(nr_pwm==21) xmlhttp.open(\"GET\",\"/mobile/pk011\", true);\r\n\
if(nr_pwm==22) xmlhttp.open(\"GET\",\"/mobile/pk012\", true);\r\n\
if(nr_pwm==23) xmlhttp.open(\"GET\",\"/mobile/pk013\", true);\r\n\
if(nr_pwm==24) xmlhttp.open(\"GET\",\"/mobile/pk007\", true);\r\n\
if(nr_pwm==25) xmlhttp.open(\"GET\",\"/mobile/pk008\", true);\r\n\
if(nr_pwm==26) xmlhttp.open(\"GET\",\"/mobile/pk009\", true);\r\n\
if(nr_pwm==27) xmlhttp.open(\"GET\",\"/mobile/pk010\", true);\r\n\
if(nr_pwm==28) xmlhttp.open(\"GET\",\"/mobile/pk001\", true);\r\n\
if(nr_pwm==29) xmlhttp.open(\"GET\",\"/mobile/pk002\", true);\r\n\
if(nr_pwm==30) xmlhttp.open(\"GET\",\"/mobile/pk003\", true);\r\n\
if(nr_pwm==31) xmlhttp.open(\"GET\",\"/mobile/pk004\", true);\r\n\
if(nr_pwm==32) xmlhttp.open(\"GET\",\"/mobile/pk005\", true);\r\n\
if(nr_pwm==33) xmlhttp.open(\"GET\",\"/mobile/pk006\", true);\r\n\
xmlhttp.send();\r\n\
}\r\n\
function xpk021(){if(wst==0){wst=1;now=0;nr_pwm=21;loadXMLDoc_PK(); }}\r\n\
function xpk022(){if(wst==0){wst=1;now=0;nr_pwm=22;loadXMLDoc_PK(); }}\r\n\
function xpk023(){if(wst==0){wst=1;now=0;nr_pwm=23;loadXMLDoc_PK(); }}\r\n\
function xpk024(){if(wst==0){wst=1;now=0;nr_pwm=24;loadXMLDoc_PK(); }}\r\n\
function xpk025(){if(wst==0){wst=1;now=0;nr_pwm=25;loadXMLDoc_PK(); }}\r\n\
function xpk026(){if(wst==0){wst=1;now=0;nr_pwm=26;loadXMLDoc_PK(); }}\r\n\
function xpk027(){if(wst==0){wst=1;now=0;nr_pwm=27;loadXMLDoc_PK(); }}\r\n\
function xpk028(){if(wst==0){wst=1;now=0;nr_pwm=28;loadXMLDoc_PK(); }}\r\n\
function xpk029(){if(wst==0){wst=1;now=0;nr_pwm=29;loadXMLDoc_PK(); }}\r\n\
function xpk030(){if(wst==0){wst=1;now=0;nr_pwm=30;loadXMLDoc_PK(); }}\r\n\
function xpk031(){if(wst==0){wst=1;now=0;nr_pwm=31;loadXMLDoc_PK(); }}\r\n\
function xpk032(){if(wst==0){wst=1;now=0;nr_pwm=32;loadXMLDoc_PK(); }}\r\n\
function xpk033(){if(wst==0){wst=1;now=0;nr_pwm=33;loadXMLDoc_PK(); }}\r\n\
function outputUpdate021(vol){document.querySelector('#volume021').value=vol;}function x021(val){if(wst==0){wst=1;now=0;nr_pwm=1; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
function outputUpdate022(vol){document.querySelector('#volume022').value=vol;}function x022(val){if(wst==0){wst=1;now=0;nr_pwm=2; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
function outputUpdate023(vol){document.querySelector('#volume023').value=vol;}function x023(val){if(wst==0){wst=1;now=0;nr_pwm=3; val_pwm=val; loadXMLDoc_PWM();}}\r\n\
var myVar = setInterval(loadXMLDoc, 500       );\r\n\
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
if((txt[25+13+1+0]=='0')||(txt[25+13+1+0]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we001\").innerHTML=txt3; if((txt[25+13+1+0]=='2')||(txt[25+13+1+0]=='3')) document.getElementById(\"tu_lora001we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+1]=='0')||(txt[25+13+1+1]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we002\").innerHTML=txt3; if((txt[25+13+1+1]=='2')||(txt[25+13+1+1]=='3')) document.getElementById(\"tu_lora002we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+2]=='0')||(txt[25+13+1+2]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we003\").innerHTML=txt3; if((txt[25+13+1+2]=='2')||(txt[25+13+1+2]=='3')) document.getElementById(\"tu_lora003we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+3]=='0')||(txt[25+13+1+3]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we004\").innerHTML=txt3; if((txt[25+13+1+3]=='2')||(txt[25+13+1+3]=='3')) document.getElementById(\"tu_lora004we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+4]=='0')||(txt[25+13+1+4]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we005\").innerHTML=txt3; if((txt[25+13+1+4]=='2')||(txt[25+13+1+4]=='3')) document.getElementById(\"tu_lora005we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+5]=='0')||(txt[25+13+1+5]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we006\").innerHTML=txt3; if((txt[25+13+1+5]=='2')||(txt[25+13+1+5]=='3')) document.getElementById(\"tu_lora006we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+6]=='0')||(txt[25+13+1+6]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we007\").innerHTML=txt3; if((txt[25+13+1+6]=='2')||(txt[25+13+1+6]=='3')) document.getElementById(\"tu_lora007we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+7]=='0')||(txt[25+13+1+7]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we008\").innerHTML=txt3; if((txt[25+13+1+7]=='2')||(txt[25+13+1+7]=='3')) document.getElementById(\"tu_lora008we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+8]=='0')||(txt[25+13+1+8]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we009\").innerHTML=txt3; if((txt[25+13+1+8]=='2')||(txt[25+13+1+8]=='3')) document.getElementById(\"tu_lora009we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+9]=='0')||(txt[25+13+1+9]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we010\").innerHTML=txt3; if((txt[25+13+1+9]=='2')||(txt[25+13+1+9]=='3')) document.getElementById(\"tu_lora010we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+10]=='0')||(txt[25+13+1+10]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we011\").innerHTML=txt3; if((txt[25+13+1+10]=='2')||(txt[25+13+1+10]=='3')) document.getElementById(\"tu_lora011we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+11]=='0')||(txt[25+13+1+11]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we012\").innerHTML=txt3; if((txt[25+13+1+11]=='2')||(txt[25+13+1+11]=='3')) document.getElementById(\"tu_lora012we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+12]=='0')||(txt[25+13+1+12]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we013\").innerHTML=txt3; if((txt[25+13+1+12]=='2')||(txt[25+13+1+12]=='3')) document.getElementById(\"tu_lora013we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+13]=='0')||(txt[25+13+1+13]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we014\").innerHTML=txt3; if((txt[25+13+1+13]=='2')||(txt[25+13+1+13]=='3')) document.getElementById(\"tu_lora014we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+14]=='0')||(txt[25+13+1+14]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we017\").innerHTML=txt3; if((txt[25+13+1+14]=='2')||(txt[25+13+1+14]=='3')) document.getElementById(\"tu_lora017we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+15]=='0')||(txt[25+13+1+15]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we018\").innerHTML=txt3; if((txt[25+13+1+15]=='2')||(txt[25+13+1+15]=='3')) document.getElementById(\"tu_lora018we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+16]=='0')||(txt[25+13+1+16]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we019\").innerHTML=txt3; if((txt[25+13+1+16]=='2')||(txt[25+13+1+16]=='3')) document.getElementById(\"tu_lora019we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019we\").innerHTML=\" \";\r\n\
if((txt[25+13+1+17]=='0')||(txt[25+13+1+17]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_we020\").innerHTML=txt3; if((txt[25+13+1+17]=='2')||(txt[25+13+1+17]=='3')) document.getElementById(\"tu_lora020we\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020we\").innerHTML=\" \";\r\n\
\r\n\
if((txt[25+13+1+18+1+0]=='5')||(txt[25+13+1+18+1+0]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+0]=='0')||(txt[25+13+1+18+1+0]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol001\").innerHTML=txt3; if((txt[25+13+1+18+1+0]=='2')||(txt[25+13+1+18+1+0]=='3')) document.getElementById(\"tu_lora001eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+1]=='5')||(txt[25+13+1+18+1+1]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+1]=='0')||(txt[25+13+1+18+1+1]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol002\").innerHTML=txt3; if((txt[25+13+1+18+1+1]=='2')||(txt[25+13+1+18+1+1]=='3')) document.getElementById(\"tu_lora002eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+2]=='5')||(txt[25+13+1+18+1+2]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+2]=='0')||(txt[25+13+1+18+1+2]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol003\").innerHTML=txt3; if((txt[25+13+1+18+1+2]=='2')||(txt[25+13+1+18+1+2]=='3')) document.getElementById(\"tu_lora003eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+3]=='5')||(txt[25+13+1+18+1+3]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+3]=='0')||(txt[25+13+1+18+1+3]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol004\").innerHTML=txt3; if((txt[25+13+1+18+1+3]=='2')||(txt[25+13+1+18+1+3]=='3')) document.getElementById(\"tu_lora004eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+4]=='5')||(txt[25+13+1+18+1+4]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+4]=='0')||(txt[25+13+1+18+1+4]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol005\").innerHTML=txt3; if((txt[25+13+1+18+1+4]=='2')||(txt[25+13+1+18+1+4]=='3')) document.getElementById(\"tu_lora005eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+5]=='5')||(txt[25+13+1+18+1+5]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+5]=='0')||(txt[25+13+1+18+1+5]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol006\").innerHTML=txt3; if((txt[25+13+1+18+1+5]=='2')||(txt[25+13+1+18+1+5]=='3')) document.getElementById(\"tu_lora006eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+6]=='5')||(txt[25+13+1+18+1+6]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+6]=='0')||(txt[25+13+1+18+1+6]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol007\").innerHTML=txt3; if((txt[25+13+1+18+1+6]=='2')||(txt[25+13+1+18+1+6]=='3')) document.getElementById(\"tu_lora007eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+7]=='5')||(txt[25+13+1+18+1+7]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+7]=='0')||(txt[25+13+1+18+1+7]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol008\").innerHTML=txt3; if((txt[25+13+1+18+1+7]=='2')||(txt[25+13+1+18+1+7]=='3')) document.getElementById(\"tu_lora008eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+8]=='5')||(txt[25+13+1+18+1+8]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+8]=='0')||(txt[25+13+1+18+1+8]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol009\").innerHTML=txt3; if((txt[25+13+1+18+1+8]=='2')||(txt[25+13+1+18+1+8]=='3')) document.getElementById(\"tu_lora009eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+9]=='5')||(txt[25+13+1+18+1+9]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+9]=='0')||(txt[25+13+1+18+1+9]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol010\").innerHTML=txt3; if((txt[25+13+1+18+1+9]=='2')||(txt[25+13+1+18+1+9]=='3')) document.getElementById(\"tu_lora010eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+10]=='5')||(txt[25+13+1+18+1+10]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+10]=='0')||(txt[25+13+1+18+1+10]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol011\").innerHTML=txt3; if((txt[25+13+1+18+1+10]=='2')||(txt[25+13+1+18+1+10]=='3')) document.getElementById(\"tu_lora011eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+11]=='5')||(txt[25+13+1+18+1+11]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+11]=='0')||(txt[25+13+1+18+1+11]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol012\").innerHTML=txt3; if((txt[25+13+1+18+1+11]=='2')||(txt[25+13+1+18+1+11]=='3')) document.getElementById(\"tu_lora012eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+12]=='5')||(txt[25+13+1+18+1+12]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+12]=='0')||(txt[25+13+1+18+1+12]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol013\").innerHTML=txt3; if((txt[25+13+1+18+1+12]=='2')||(txt[25+13+1+18+1+12]=='3')) document.getElementById(\"tu_lora013eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013eol\").innerHTML=\" \";\r\n\
if((txt[25+13+1+18+1+13]=='5')||(txt[25+13+1+18+1+13]=='6')) txt3=\"<font color='#d00'>Uszkodzona linia     </font>\"; else if((txt[25+13+1+18+1+13]=='0')||(txt[25+13+1+18+1+13]=='2')) txt3=\"<font color='#d00'>Stan ON              </font>\"; else txt3=\"<font color='#000'>Stan OFF             </font>\";  document.getElementById(\"tu_eol014\").innerHTML=txt3; if((txt[25+13+1+18+1+13]=='2')||(txt[25+13+1+18+1+13]=='3')) document.getElementById(\"tu_lora014eol\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014eol\").innerHTML=\" \";\r\n\
\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp001a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora001a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp001b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora001b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora001b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+0+7+i];txt4[5]=txt[25+13+1+18+1+14+1+0+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+0+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp001c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp002a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora002a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp002b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora002b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora002b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+12+7+i];txt4[5]=txt[25+13+1+18+1+14+1+12+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+12+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp002c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp003a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora003a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp003b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora003b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora003b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+24+7+i];txt4[5]=txt[25+13+1+18+1+14+1+24+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+24+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp003c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp004a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora004a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp004b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora004b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora004b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+36+7+i];txt4[5]=txt[25+13+1+18+1+14+1+36+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+36+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp004c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp005a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp005b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora005b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora005b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+48+7+i];txt4[5]=txt[25+13+1+18+1+14+1+48+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+48+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp005c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp006a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp006b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora006b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora006b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+60+7+i];txt4[5]=txt[25+13+1+18+1+14+1+60+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+60+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp006c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp007a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp007b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora007b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora007b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+72+7+i];txt4[5]=txt[25+13+1+18+1+14+1+72+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+72+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp007c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp008a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp008b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora008b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora008b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+84+7+i];txt4[5]=txt[25+13+1+18+1+14+1+84+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+84+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp008c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp009a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp009b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora009b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora009b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+96+7+i];txt4[5]=txt[25+13+1+18+1+14+1+96+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+96+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp009c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp010a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp010b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora010b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora010b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+108+7+i];txt4[5]=txt[25+13+1+18+1+14+1+108+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+108+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp010c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp011a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora011a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp011b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora011b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora011b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+120+7+i];txt4[5]=txt[25+13+1+18+1+14+1+120+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+120+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp011c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp012a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora012a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp012b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora012b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora012b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+132+7+i];txt4[5]=txt[25+13+1+18+1+14+1+132+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+132+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp012c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp013a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora013a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp013b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora013b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora013b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+144+7+i];txt4[5]=txt[25+13+1+18+1+14+1+144+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+144+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp013c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp014a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora014a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp014b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora014b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora014b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+156+7+i];txt4[5]=txt[25+13+1+18+1+14+1+156+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+156+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp014c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp017a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora017a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp017b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora017b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora017b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+168+7+i];txt4[5]=txt[25+13+1+18+1+14+1+168+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+168+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp017c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp018a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora018a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp018b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora018b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora018b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+180+7+i];txt4[5]=txt[25+13+1+18+1+14+1+180+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+180+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp018c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp019a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora019a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp019b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora019b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora019b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+192+7+i];txt4[5]=txt[25+13+1+18+1+14+1+192+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+192+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp019c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
  for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp020a\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora020a\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020a\").innerHTML=\" \";\r\n\
 for(i=0;i<7;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+i];if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp020b\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+\" \"+txt4[1]+txt4[2]+txt4[3]+txt4[4]+\" °C\"+\"</font>\";if(txt4[6]=='x') document.getElementById(\"tu_lora020b\").innerHTML=\"<div id='tu_ant'></div>\"; else document.getElementById(\"tu_lora020b\").innerHTML=\" \";\r\n\
 for(i=0;i<2;i++) txt4[i]=txt[25+13+1+18+1+14+1+204+7+i];txt4[5]=txt[25+13+1+18+1+14+1+204+7+3]; txt4[i++]=' ';txt4[i]=txt[25+13+1+18+1+14+1+204+7+2]; if(txt4[5]=='1') ccol=\"d00\";if(txt4[5]=='0') ccol=\"000\";if(txt4[5]=='2') ccol=\"00d\"; document.getElementById(\"tu_temp020c\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+\"</font>\";\r\n\
 \r\n\
 for(i=0;i<5;i++) txt5[i]=txt[25+13+1+18+1+14+1+216+1+20+1+140+1+0+i];for(i=0;i<9;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+0+i];document.getElementById(\"tu_adc015\").innerHTML=txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" \"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4];\r\n\
for(i=0;i<5;i++) txt5[i]=txt[25+13+1+18+1+14+1+216+1+20+1+140+1+6+i];for(i=0;i<9;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+10+i];document.getElementById(\"tu_adc016\").innerHTML=txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" \"+txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4];\r\n\
\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+0+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt001\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+10+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt002\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+20+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt003\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+30+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt004\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+40+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt005\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+50+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt006\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+60+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt007\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+70+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt008\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+80+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt009\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+90+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt010\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+100+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt011\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+110+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt012\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+120+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt013\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
for(i=0;i<10;i++) txt4[i]=txt[25+13+1+18+1+14+1+216+1+130+1+20+i];if(txt4[9]=='1') ccol=\"d00\";if(txt4[9]=='0') ccol=\"000\";if(txt4[9]=='2') ccol=\"00d\";document.getElementById(\"tu_ptt014\").innerHTML=\"<font color='#\"+ccol+\"'>\"+txt4[0]+txt4[1]+txt4[2]+txt4[3]+txt4[4]+txt4[5]+txt4[6]+txt4[7]+txt4[8]+\" °C\";\r\n\
\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*0+4*18*0+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_017\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*1+4*18*1+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_018\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*2+4*18*2+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_019\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n\
j=0;for(i=0;i<22;i++){ txt5[i]=txt[450+14+1+23*3+4*18*3+i]; if((txt5[i]=='0')&&(j==0))txt5[i]=' ';else j=1;}document.getElementById(\"tu_imp0_020\").innerHTML=txt5[0]+txt5[1]+txt5[2]+txt5[3]+txt5[4]+txt5[5]+txt5[6]+txt5[7]+txt5[8]+txt5[9]+txt5[10]+txt5[11]+txt5[12]+txt5[13]+txt5[14]+txt5[15]+txt5[16]+txt5[17]+txt5[18]+txt5[19]+txt5[20];\r\n\
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
  <style>\r\n\
­iv { margin: 20px; }\r\n\
 #tu_ant {\r\n\
padding: 15px 12px;\r\n\
margin-top: 11px;\r\n\
border-radius: 6px;\r\n\
		background:   url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABkAAAAcCAYAAACUJBTQAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAgY0hSTQAAeiYAAICEAAD6AAAAgOgAAHUwAADqYAAAOpgAABdwnLpRPAAAAAlwSFlzAAAOxAAADsQBlSsOGwAABsFJREFUSEvtVl2sVNUV/vb5n3POzL2DXFDEolRQxKqtQmpbQ6Vogj/EmCCmoQkvVvx56F+I7UsfeaFNSlJjK02QqkmLL6Y19aeNQdT6oKA4pEKsjbfQcJF75947d+bM+dl791tnuNHXvndy150ze6+91vrW+tbaRw1OPWAd5QDWhzIelAUcaCiUXKvAfxTuf0GscrnswCgu84BjqaPkjOhq7slvC+MoDLWB4xYKrq4oQ7hVDkeX1KcDEfOFb3k2Fc9rGDqvVAEDii1RMSBZM9SxRpwY6lrYokLiMwivaMKpXLheSOeyqQmAimC0bgRNdFlhUVqFyndRcjmj2Zy6mQQXBFCeyxWForIoyovIec41CWwmIN/9ri2cWXiRT5spytyiKjyUJkYZLMcnk9PofPgxPjo1ifNT8wwCGGtGGG9FWH3lBG77+vVYPtGgiwyRr1EM56GZkUYzBXIJeoFOjm23KmEq/ABTnxVI0+Vw3aV48aX38Jvn/47JczlmZys41A8CoskIn2mQKil4aI47WHNVjJ07N2DrnV/l2QGG+TQznWM8ipn+AZ0c32FtajHVLXHZyhvx0p9P4Bf7XsPp0z1UqYdhxUS4hE4HaeLCdxV0YaAZZTnPvHM/afkYzpe4+sYWnvjZd7Dh1hWI3RyYXUBU5CMnXTIjTFbgt0+/hV8/dQIDHgjGeHCuxM23XYpN316PdeuWYemyCHEjRNY3mLlQ4IP3zjKYKbz8pxM1r7y2D03Gff+xa7B712Y0h9NISkHywYPWtCewd9/LeGr/v+CMkXZDi1tuaWLvnq1YdXkD40tZQLKvn7EmYs5xkFdMmMt1m+DkP85h3/5X8O77Ug9mPlbYtfMm7HnoK2jqWVKcKLK5DG8dOY8oVTDM+U+f+CYOHXgc69YEiNwZZLNTjEYjjZcg9McRuDHarQTNgOQdnsHNN4zj8DMP4yePrkfkMQzaOPLqR2Qr00lRCye2WU+neOPNU3jymX/iwR2r8MD2OzAzPcUmyhGQcZ9MZjjeOY93jk1iQDq30wauvnIJtnxjLZaOGbQYeT7sIRqL8crrHex/8jR2fW8ttm25CqEdQvU622xcNIAwIHzDHmJRS82+CDC94OLAoaM4/OJZzMyxf0aNDsuapolC03Ow/e7LseeH9yEb/AftCYVBPiTado2iyubYbSROr3OvDfseFrICzYkmev0+WmmMf59TeOjHf0HnZB+GlJWPz1wYoT5FPiwNFPe+fG0TB5/ejbHWZ2Rfv15jPEwrFZWhk5P32LgkEk3PHAFKKMp5s28/++TZs0xDRWo3cP+2Dbhj89d4aIBS5zjy9nE8/8fTJImui33f3Zdi78/vRb/7KSbYrGwWZobzzWVfCVsqQgT7wPUUFLuO5cdNN4xBcW3jpkvwh+d24PFHrsH6tV3cuhG4fl0Pux++Fi8cvger1yT17Lx90xVI4hITaQj0B0DFBpfxQPtqoXOX9XsOvFaDUQ1QMKwkCsn3AGe6Bu1lbRTZNC7hftG36HbnsfK6FejPzkK7LrKMA3Posfkc+MUAfp7B80jzmDXmLNNkG51stf6chdcIkLNIUeKzJyzTRBLAZyEcIjK4cKHEoYMdvHFkGhs3jhHJFsTpgB0/QDtJYPolRw87kTVAQCfsdEmXrdNFFnhS0IJdToNlUWB+ro+QtWkwooJE8Dhlf/dchxPhDD7+NMPvnz2HAwePopWMU4eN2Z2jIUmPEEQz/RmMoHFkDtTZ5ANp67AeMofkHkhjnwziuOefUj4y4+DNY3P1iK+SGEPy5Oj7F9CnusMAwkjGJX+4JEFI+nMAD0ODSqaqNKN+53biIUyBWjsWyCIj5xy06JGzXrQMP/jR6/jbX7vYvHUJfvXLzbD9GYS8yFy5Cesak7hixmN0RCG9knkekdR3qGjIRTW6NrVIDVScOYyBWa0MGoGDtOkgDl3kg4JBisXFACUpn4vEJyhkrPC0OBnZE++GoglTRFBo3pLyCqDl+iXCvMc9EiNu8H2AwdHM6GydhtGzAFs0ygzSiaRlUYeohCEuayBSp5k5l6hch0zj6ZBIXJJikPG+l+O11cUoR8+S7UWjmhn8P5L/sSaf12hUNNbIspGsxztc3r3YsQmv4qChsXJVhMtWR7jiSxHzznnncKQIQUSoJ1eFvORZ+S2fEZM5hd/+lo0duSikWheZMnoVucgYeeZkFi5pjhz5SeMOx0fdH7UsqgrlF8/yu6BwhDk554uWJ76mMny2DB0adjwnmxjVJuA7mNwj3JOJWk8I4bQ7WqNufVbmHMXQhqaU5P8wdNBlg/4X8S+jJn4L8/oAAAAASUVORK5CYII=');        }\r\n\
</style>\r\n\
</html>";


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
