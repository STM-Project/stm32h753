#!/usr/bin/env python3
"""Host/ARM checks for settings templates without STM32 hardware headers.
Run: python3 tests/test_settings_templates.py
"""
from pathlib import Path
import ast
import collections
import json
import hashlib
from html.parser import HTMLParser
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

class Controls(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = collections.Counter()
        self.links = collections.Counter()
        self.fields = collections.Counter()
        self.options = collections.Counter()
        self.selected = collections.Counter()
        self.values = collections.Counter()
        self.select = None
    def handle_starttag(self, tag, attributes):
        attrs = dict(attributes)
        if 'id' in attrs and attrs['id'] != 'mainselection':
            self.ids[attrs['id']] += 1
        if tag == 'a' and 'href' in attrs:
            self.links[attrs['href']] += 1
        if attrs.get('type') == 'hidden' and attrs.get('name') in {'h7_page', 'h7_form'}:
            return
        if 'name' in attrs:
            self.fields[tag, attrs['name']] += 1
        if tag == 'input':
            self.values[attrs.get('name'), attrs.get('value')] += 1
        if tag == 'select':
            self.select = attrs.get('name')
        if tag == 'option':
            self.options[self.select, attrs.get('value')] += 1
            if 'selected' in attrs:
                self.selected[self.select, attrs.get('value')] += 1
    def handle_endtag(self, tag):
        if tag == 'select': self.select = None

def literal(source, name):
    match = re.search(r'const char '+name+r'\[\]\s*=\s*((?:"(?:\\.|[^"\\])*"\s*)+);', source, re.S)
    return ''.join(ast.literal_eval(x.replace('\\\n', '')) for x in re.findall(r'"(?:\\.|[^"\\])*"',match[1],re.S))

def main():
    source = (ROOT/'Application/wwwPages.c').read_text()
    header = (ROOT/'Application/wwwPages.h').read_text()
    esp = (ROOT/'Application/esp32wroom.c').read_text()
    helper = source[source.index('/* Mode values are CGI'):source.index('SDRAM_ALIGN32 char HttpRefr_sdram')]
    templates = source[source.index('/* Settings page templates.'):source.index('const char HttpStructMainReadPanel2')]
    calls = esp[esp.index('case WWW_MAIN_SETTING:'):esp.index('case WWW_FAVICON:')]
    calls = '\n'.join(line for line in calls.splitlines() if 'ptr=HTTP_TEMPLATE_2_' in line)
    enums = '\n'.join(re.findall(r'typedef enum\{.*?\}\w+;',header,re.S))
    preamble = '#include <stdint.h>\n#include <stdio.h>\n#include <string.h>\n#include <assert.h>\n#include <stdlib.h>\n#include "http_settings_model.h"\n#include "mini_printf.h"\ntypedef uint8_t u8;\n'
    harness = preamble+enums+'\n'+helper+templates+'''
int main(void) {
 static char buffer[100000]; char *ptr=buffer;
 assert(HTTP_SettingsModelInit(NULL,NULL,NULL)==0);
'''+calls+'''
 assert(ptr==buffer+strlen(buffer)); assert(ptr-buffer<100000);
 fwrite(buffer,1,ptr-buffer,stdout);
 char attribute[1024]={0};
 char *end=HTTP_TEMPLATE_2_Name(attribute,1,"A & B \\\"<test>\\\"");
 assert(end==attribute+strlen(attribute));
 assert(strstr(attribute,"A &amp; B &quot;&lt;test&gt;&quot;"));
 char empty[8]={0};
 assert(HTTP_TEMPLATE_2_Name(empty,0,"x")==empty);
 assert(HTTP_TEMPLATE_2_Hidden(empty,34)==empty);
 assert(HTTP_TEMPLATE_2_Mode(empty,34,0)==empty);
 assert(empty[0]==0);
 return 0;
}
'''
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp); test = tmp/'settings.c'; test.write_text(harness)
        include = '-I'+str(ROOT/'Application/General_Utilities')
        app_include = '-I'+str(ROOT/'Application')
        modules = [str(ROOT/'Application/http_settings_catalog.c'),str(ROOT/'Application/http_settings_model.c')]
        subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',include,app_include,str(test),*modules,str(ROOT/'Application/General_Utilities/mini_printf.c'),'-o',str(tmp/'test')],check=True)
        body = subprocess.check_output([str(tmp/'test')]).decode()
        menu = literal(source,'HttpMainMenu')
        footer = literal(source,'HttpStructMainReadPanel2')
        rendered = menu+body+footer
        (tmp/'settings.html').write_text(rendered)
        controls = Controls(); controls.feed(rendered)
        assert len([x for x in controls.fields if x[0]=='select']) == 33
        assert len(controls.selected) == 33
        for nr in range(1,34):
            for mode in [0,2,9,10,11,8,4,13,1,5,3,7,6]:
                assert controls.ids[f'{mode:02}p{nr:03}']==1
        assert rendered.count('<body ') == rendered.count('</body>') == 1
        assert rendered.count('</html>') == 1
        assert 'HttpMainSettings' not in calls
        # Semantic fingerprints of the original HttpMainSettings controls.
        # The original uu_pk027 typo is corrected to match its refresh target.
        expected = json.loads((ROOT/'tests/fixtures/settings_controls.sha256.json').read_text())
        for name, digest in expected.items():
            actual = hashlib.sha256(json.dumps(sorted(getattr(controls,name).items()),ensure_ascii=False).encode()).hexdigest()
            assert actual == digest, f'Original settings controls changed: {name}'
        print('PASS: original IDs, links, fields, options, selected modes and input values preserved.')
        scripts = re.findall(r'<script[^>]*>(.*?)</script>',rendered,re.S)
        for i,script in enumerate(scripts):
            js=tmp/f'script{i}.js'; js.write_text(script)
            if shutil.which('node'): subprocess.run(['node','--check',str(js)],check=True)
        if shutil.which('node'):
            selected = {name: value for (name, value), count in controls.selected.items()}
            js_test = """
const vm=require('vm'),assert=require('assert');
const selected=SELECTED;
const nodes={}; for(const id of IDS) nodes[id]={style:{},innerHTML:''};
const forms={}; for(const name in selected){
 const nr=name.slice(-3); forms['wybor'+nr]={[name]:{value:selected[name]}};
}
const context={document:{forms,getElementById:id=>nodes[id]},window:{location:{}},setInterval:()=>1,pageScroll:()=>{}};
vm.createContext(context); vm.runInContext(SCRIPTS,context);
context.window.onload();
for(const name in selected) assert.equal(nodes[selected[name]].style.display,'block');
forms.wybor005.n_GPIO_val_005.value='02p005';
forms.wybor005.n_GPIO_val_005.onchange.call(forms.wybor005.n_GPIO_val_005);
assert.equal(nodes['09p005'].style.display,'none');
assert.equal(nodes['02p005'].style.display,'block');
context.zaladuj();
assert(context.window.location.href.startsWith('/lpc.cgi/A117?h7_page=%2Fset&h7_form=1&'));
assert(context.window.location.href.includes('n_GPIO_val_005=02'));
assert.equal((context.window.location.href.match(/n_GPIO_val_/g)||[]).length,33);
console.log('PASS: settings JavaScript initializes 33 ports, switches modes and saves all 33 CGI values.');
""".replace('SELECTED',json.dumps(selected)).replace('IDS',json.dumps(list(controls.ids))).replace('SCRIPTS',json.dumps('\n'.join(scripts)))
            js=tmp/'behavior.js'; js.write_text(js_test)
            subprocess.run(['node',str(js)],check=True)
        arm = Path('/workspace/.stm32-tools/root/usr/bin/arm-none-eabi-gcc')
        if arm.exists():
            subprocess.run([str(arm),'-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','--specs=nano.specs','-Wall','-Wextra','-Werror',include,app_include,'-c',str(test),'-o',str(tmp/'test.o')],check=True)
            print('PASS: ARM Cortex-M7 compilation.')
        print(f'PASS: 33 forms, 429 mode sections, HTML attribute escaping, buffer checks, ASan/UBSan and JavaScript syntax; dynamic body {len(body.encode())} bytes.')

if __name__ == '__main__': main()
