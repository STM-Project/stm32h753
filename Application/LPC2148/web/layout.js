/* Align matching rows in legacy tables arranged as parallel columns. */
(function(){
 'use strict';
 var queued=false;
 function align(){
  queued=false;
  document.querySelectorAll('#lpc-content .lpc-data-table tr').forEach(function(row){
   var tables=[];
   Array.prototype.forEach.call(row.children,function(cell){
    Array.prototype.forEach.call(cell.children,function(child){if(child.tagName==='TABLE')tables.push(child);});
   });
   if(tables.length<2)return;
   var count=tables[0].rows.length;
   if(count<3||tables.some(function(table){return table.rows.length!==count;}))return;
   tables.forEach(function(table){Array.prototype.forEach.call(table.rows,function(tr){tr.style.height='';});});
   for(var i=0;i<count;i++){
    var height=0;
    tables.forEach(function(table){height=Math.max(height,table.rows[i].getBoundingClientRect().height);});
    tables.forEach(function(table){table.rows[i].style.height=Math.ceil(height)+'px';});
   }
  });
 }
 function schedule(){if(!queued){queued=true;window.requestAnimationFrame(align);}}
 if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',schedule);else schedule();
 window.addEventListener('load',schedule);
 window.addEventListener('resize',schedule);
 window.addEventListener('lpc-layout',schedule);
}());
