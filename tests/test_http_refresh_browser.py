"""Run the actual legacy TME poll functions with a slow/failing XMLHttpRequest."""
from pathlib import Path
import ast,json,re,subprocess
root=Path(__file__).resolve().parents[1]
s=(root/'Application/wwwPages.c').read_text()
pattern=re.compile(r'const char\s+(\w+)\[\]\s*=\s*((?:"(?:\\.|[^"\\])*"\s*)+);',re.S)
checked=[]
for match in pattern.finditer(s):
 parts=re.findall(r'"(?:\\.|[^"\\])*"',match[2],re.S)
 text=''.join(ast.literal_eval(p.replace('\\\n','')) for p in parts)
 start=text.find('var httpRefreshBusy=false,httpRefreshRequest=null; function loadXMLDoc()')
 if start<0:continue
 end=text.find('</script>',start)
 code=text[start:end]
 test=r'''
const vm=require('vm'),assert=require('assert');
const sent=[];
function XHR(){this.readyState=0;this.status=0;}
XHR.prototype.open=function(method,url){assert.equal(method,'GET');assert.equal(url,'/TME.txt');};
XHR.prototype.send=function(){sent.push(this);};
const ctx={window:{XMLHttpRequest:XHR},XMLHttpRequest:XHR,wst:0,now:0};
vm.createContext(ctx);vm.runInContext(CODE,ctx);
function poll(){ctx.wst=0;ctx.loadXMLDoc();}
poll();assert.equal(sent.length,1);
for(let i=0;i<30;i++)poll();assert.equal(sent.length,1);
sent[0].readyState=2;sent[0].onreadystatechange();poll();assert.equal(sent.length,1);
sent[0].readyState=4;sent[0].status=503;sent[0].onreadystatechange();poll();assert.equal(sent.length,2);
sent[0].ontimeout();sent[0].onerror();poll();assert.equal(sent.length,2);
assert.equal(sent[1].timeout,10000);sent[1].ontimeout();poll();assert.equal(sent.length,3);
sent[2].onerror();poll();assert.equal(sent.length,4);
sent[3].onabort();poll();assert.equal(sent.length,5);
'''.replace('CODE',json.dumps(code))
 subprocess.run(['node','-e',test],check=True);checked.append(match[1])
assert len(checked)==3,checked
print('Legacy refresh: three real pollers allow one in-flight GET, skip timer backlog, recover after HTTP error/timeout/error/abort: PASS')
