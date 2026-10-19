/* Safe adapter for imported LPC2148 layouts. Legacy executable scripts are removed. */
(function(){
'use strict';
var fields={},busy=false,charts={},widgets={},pending={};
function status(s){document.getElementById('lpc-status').textContent=s;}
function text(id,value){var e=document.getElementById(id);if(e)e.textContent=value;}
function pad(n){return ('00'+n).slice(-3);}
function fieldValue(name){return Object.prototype.hasOwnProperty.call(fields,name)?fields[name]:null;}
function variables(){
 document.querySelectorAll('[data-lpc-variable]').forEach(function(e){
  var name=e.getAttribute('data-lpc-variable'),value=fieldValue(name.replace(/^v_/,'n_'));
  if(value!==null)e.textContent=value;
  else if(/^_Size/.test(name))e.textContent='20';
 });
}
function refreshControls(){
 document.querySelectorAll('[data-lpc-field]').forEach(function(e){
  var value=fieldValue(e.name);if(value===null)return;
  if(e.type==='checkbox')e.checked=value==='1'||value==='tak'||value==='on';
  else if(e.type==='radio')e.checked=e.value===value;
  else e.value=value;
  try{if(e.type==='checkbox'||e.type==='radio')$(e).checkboxradio('refresh');else if(e.tagName==='SELECT')$(e).selectmenu('refresh');}catch(ignore){}
 });variables();window.dispatchEvent(new Event('lpc-layout'));
}
function loadSettings(){
 return $.ajax({url:'/api/lpc'+window.LPC_PAGE,dataType:'json',cache:true,timeout:10000}).done(function(data){fields=data.fields;refreshControls();status('Ustawienia LPC wczytane.');}).fail(function(x){status('Nie wczytano ustawień LPC: HTTP '+x.status);});
}
function colour(v,range){var p=Math.max(0,Math.min(1,v/range));return 'rgb('+Math.round(255*p)+','+Math.round(128*(1-p))+','+Math.round(255*(1-p))+')';}
function live(data){
 var now=Date.now();
 data.temperatures.forEach(function(item,i){
  var n=i+1,id=pad(n),value=item.tenths===null?null:item.tenths/10,label=value===null?'brak danych':value.toFixed(1);
  text('tu_temp'+id,label);text('id_txt_temp'+n,item.name);
  document.querySelectorAll('[data-lpc-variable="v_txtval'+id+'"]').forEach(function(e){e.textContent=fieldValue('n_txtval'+id)||item.name;});
  var fixture=n===1?'fixture':'fixture'+n,node=document.getElementById(fixture),lo=item.min/10,hi=item.max/10;if(!(hi>lo)){lo=-10;hi=50;}
  if(node){try{var range=hi-lo,key=lo+'/'+hi;if(widgets[fixture]&&widgets[fixture]!==key){$(node).thermometer('destroy').empty();delete widgets[fixture];}if(!widgets[fixture]){$(node).thermometer({minValue:0,maxValue:range,height:150,width:'100%',svgMarkup:window.HTTP_LPC_SVG,bottomText:String(lo),topText:String(hi),animationSpeed:200,liquidColour:function(v){return colour(v,range);}});widgets[fixture]=key;}$(node).thermometer('setValue',value===null?0:value-lo);node.style.opacity=value===null?'.4':'1';}catch(ignore){}}
  Object.keys(charts).forEach(function(key){var chart=charts[key];if(chart.series[i])chart.series[i].addPoint([now,value],false,chart.series[i].data.length>=300);});
 });
 data.inputs.forEach(function(item,i){var id=pad(i+1);text('tu_we'+id,item.state===null?'brak danych':item.state?'ON':'OFF');text('tu_we_p'+id,'Port '+item.port);text('id_txt_we'+(i+1),item.name);document.querySelectorAll('[data-lpc-variable="v_txtwe'+id+'"]').forEach(function(e){e.textContent=fieldValue('n_txtwe'+id)||item.name;});});
 data.outputs.forEach(function(item,i){var id=pad(i+1),node=document.getElementById('tu_pk'+id);if(node){var button=node.querySelector('button[data-live-output]');if(!button){node.textContent='';button=document.createElement('button');button.type='button';button.setAttribute('data-role','none');button.setAttribute('data-live-output',String(i+1));node.appendChild(button);}button.textContent=(item.state===null?'brak danych':item.state?'ON':'OFF')+(item.requested===null?'':' · zadano '+(item.requested?'ON':'OFF'));button.disabled=!item.enabled||!!pending[i+1];}document.querySelectorAll('[data-lpc-variable="v_pktxt'+id+'"]').forEach(function(e){e.textContent=fieldValue('n_pktxt'+id)||item.name;});});
 Object.keys(charts).forEach(function(key){charts[key].redraw();});
}
function poll(){if(busy)return;busy=true;$.ajax({url:'/api/lpcdata',dataType:'json',cache:true,timeout:10000}).done(function(data){try{live(data);}catch(e){status('Błąd widoku: '+e.message);}}).fail(function(x){status('Brak odczytu SDRAM: HTTP '+x.status);}).always(function(){busy=false;});}
function command(n){if(pending[n])return;pending[n]=true;$.ajax({url:'/mobile/pk'+pad(n),timeout:10000}).done(function(){status('Polecenie PK'+n+' przyjęte.');}).fail(function(x){status('Nie wykonano polecenia PK'+n+' (HTTP '+x.status+'). Sprawdź tryb portu w /set.');}).always(function(){delete pending[n];poll();});}
$(function(){
 document.querySelectorAll('[data-role="page"]').forEach(function(e,i){if(i)e.removeAttribute('data-role');});
 document.querySelectorAll('#container,#container2').forEach(function(node){try{charts[node.id]=new Highcharts.Chart({chart:{renderTo:node.id,type:'spline',animation:false},title:{text:'Temperatury'},xAxis:{type:'datetime'},yAxis:{title:{text:'°C'}},credits:{enabled:false},plotOptions:{series:{animation:false,marker:{enabled:false}}},series:[{name:'Temperatura 1',data:[]},{name:'Temperatura 2',data:[]},{name:'Temperatura 3',data:[]}]});}catch(ignore){}});
 document.addEventListener('click',function(event){var e=event.target.closest('[data-live-output],[data-lpc-output]');if(e){event.preventDefault();event.stopPropagation();command(Number(e.getAttribute('data-live-output')||e.getAttribute('data-lpc-output')));}});
 document.querySelectorAll('form[data-lpc-form]').forEach(function(form){
  var clicked='';form.addEventListener('click',function(e){var b=e.target.closest('button,input[type=submit],input[type=image]');if(b)clicked=b.name||'';});
  form.addEventListener('submit',function(e){e.preventDefault();var number=Number(form.getAttribute('data-lpc-form')),pairs=$(form).serializeArray().filter(function(p){return window.LPC_FORM_FIELDS[number].indexOf(p.name)>=0;});pairs.push({name:'lpc_page',value:String(window.LPC_PAGE)},{name:'lpc_form',value:String(number)},{name:'lpc_submit',value:clicked});
   var buttons=form.querySelectorAll('button,input[type=submit]');buttons.forEach(function(b){b.disabled=true;});status('Zapisywanie ustawień na SD…');
   $.ajax({type:'POST',url:'/lpc.cgi/A117',data:$.param(pairs),dataType:'json',timeout:30000}).done(function(data){status(data.message);loadSettings();}).fail(function(x){var message=x.responseJSON&&x.responseJSON.message;try{if(!message)message=JSON.parse(x.responseText).message;}catch(ignore){}status(message||'Nie zapisano ustawień (HTTP '+x.status+'). Sprawdź SD.');}).always(function(){buttons.forEach(function(b){b.disabled=false;});});
  });
 });
 loadSettings().always(function(){poll();});window.setInterval(poll,1000);
});
}());
