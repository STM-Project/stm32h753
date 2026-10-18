#!/usr/bin/env python3
"""Import user-facing LPC2148 templates; never execute their legacy scripts."""
import argparse,base64,hashlib,json,re,shutil
from pathlib import Path
from lxml import html,etree
HERE=Path(__file__).resolve().parent
MARK=re.compile(r'LPCVAR_([A-Za-z0-9_]+)_END',re.I)
SKIP=('test','proby','proba','pppp','_usun','iiii','ccc','skroc')
ASSETS={'jquery','jqtouch','images','svg','css_sprites'}
def decode(p):
 b=p.read_bytes()
 try:return b.decode('utf-8-sig')
 except UnicodeDecodeError:return b.decode('cp1250')
def import_all(system):
 candidates=[]
 for base in [system/'Strona',system/'Strona mobilna']:
  if base.exists():
   candidates += [p for p in base.rglob('*') if p.suffix.lower()=='.htm' and not any(v in str(p.relative_to(base)).lower() for v in SKIP) and not any(v.lower() in ASSETS for v in p.relative_to(base).parts[:-1])]
 candidates += [p for p in system.glob('HTML*.htm') if not any(v in p.name.lower() for v in SKIP) and p.name not in ['HTML4_plik.htm']]
 candidates=sorted(set(candidates),key=lambda p:p.relative_to(system).as_posix().casefold())
 candidates=[p for p in candidates if p.relative_to(system).as_posix()!='Strona/czolowka.htm']
 priority=['czolowka2.htm','panel_prosty.htm','panel_ster_kontr.htm','panel_ster_kontr2_p.htm','panel_ster_kontr3_p.htm','panel_ster_kontr4_p.htm','panel_ster_kontr_W.htm']
 candidates.sort(key=lambda p:priority.index(p.name) if p.name in priority else len(priority))
 pages=[];fields={}
 for number,p in enumerate(candidates,1):
  source=p.relative_to(system).as_posix();text=decode(p)
  text=re.sub(r'<%([A-Za-z0-9_]+)%>',lambda m:'LPCVAR_'+m[1]+'_END',text)
  doc=html.document_fromstring(text,parser=html.HTMLParser(remove_comments=True))
  for node in list(doc.xpath('//script|//iframe|//object|//embed|//base|//link')):node.getparent().remove(node)
  body=doc.find('body')
  if body is None:body=doc
  forms=[]
  for node in body.iter():
   for key in list(node.attrib):
    if key.lower().startswith('on') or 'lpcvar_' in key.lower():del node.attrib[key]
   if node.tag=='img' or (node.tag=='input' and node.get('type','').lower()=='image'):
    src=node.get('src','')
    if src and not src.startswith('data:'):
     found=None
     for path in [p.parent/src,system/src,system/'Strona'/src,system/'Strona/Mobilna'/src]:
      if path.is_file():found=path;break
     if found:
      mime='image/png' if found.suffix.lower()=='.png' else 'image/gif' if found.suffix.lower()=='.gif' else 'image/jpeg'
      node.set('src','data:'+mime+';base64,'+base64.b64encode(found.read_bytes()).decode())
     else:node.attrib.pop('src',None);node.set('alt','')
   if node.tag=='a':
    href=node.get('href','')
    command=re.search(r'(?:pk|PK)(\d{3})(?:$|[?i])',href)
    if command and 1<=int(command[1])<=13:node.set('data-lpc-output',str(int(command[1])));node.set('href','#')
    elif href.startswith(('javascript:','http:','https:')):node.set('href','#')
  for form_id,form in enumerate(body.xpath('.//form')):
   members=[];submit=[]
   for node in form.xpath('.//input|.//select|.//textarea|.//button'):
    name=node.get('name','');kind=node.get('type','text').lower() if node.tag=='input' else node.tag
    if not name or name in ('viewport','Content-Type') or not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]{0,94}',name):continue
    if kind in ('submit','button','image','reset') or node.tag=='button':
     submit.append(name);continue
    value=node.get('value','') if node.tag=='input' else node.text or ''
    choices=[]
    if node.tag=='select':
     options=node.xpath('.//option');choices=[n.get('value',n.text or '') for n in options]
     choices=[v for v in choices if not MARK.search(v)]
     selected=[n for n in options if n.get('selected') is not None]
     value=(selected[0] if selected else options[0]).get('value','') if options else ''
    if MARK.search(value):value=''
    if kind=='checkbox':value='0'
    if kind=='password':value=''
    try:maximum=min(max(int(node.get('maxlength','256')),1),256)
    except ValueError:maximum=256
    if choices:maximum=max([len(v.encode()) for v in choices]+[maximum])
    if name not in fields:fields[name]={'name':name,'kind':'checkbox' if kind=='checkbox' else 'secret' if kind=='password' else 'select' if choices else 'text','maximum':maximum,'default':value,'choices':choices}
    else:
     f=fields[name];f['maximum']=max(f['maximum'],maximum);f['choices']=sorted(set(f['choices']+choices))
     if kind=='password':f['kind']='secret'
     elif kind!='checkbox' and f['kind']=='checkbox':f['kind']='text'
    if name not in members:members.append(name)
    node.set('data-lpc-field',name)
    if kind=='checkbox':node.attrib.pop('checked',None)
    if 'value' in node.attrib and MARK.search(node.attrib['value']):node.set('value','')
   form.set('method','post');form.set('action','/lpc.cgi/A117');form.set('data-lpc-form',str(form_id));form.set('data-ajax','false')
   forms.append({'fields':members,'submit':submit})
  # Remove tag markers from attributes; form fields are populated from JSON.
  for node in body.iter():
   for key,value in list(node.attrib.items()):
    if MARK.search(value):node.set(key,MARK.sub('',value))
  content=''.join(etree.tostring(n,encoding='unicode',method='html') for n in body)
  content=MARK.sub(lambda m:'' if m[1].startswith(('koment','scrol','_Size')) else '<span data-lpc-variable="'+m[1]+'">—</span>',content)
  if '<%' in content:content=re.sub(r'<%.*?%>','',content)
  (HERE/'source_pages'/p.relative_to(system)).parent.mkdir(parents=True,exist_ok=True)
  shutil.copyfile(p,HERE/'source_pages'/p.relative_to(system))
  title=doc.xpath('//title/text()');title=next((x.strip() for x in title if x.strip()),p.stem.replace('_',' '))
  pages.append({'number':number,'path':f'/nowa{number}','source':source,'title':title,'forms':forms,'body':content})
 schema={'pages':pages,'fields':sorted(fields.values(),key=lambda f:f['name'])}
 (HERE/'pages.json').write_text(json.dumps(schema,ensure_ascii=False,indent=2))
 print(f'Imported {len(pages)} user pages; {len(fields)} shared LPC settings fields.')
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('system',type=Path);args=parser.parse_args();import_all(args.system)
if __name__=='__main__':main()
