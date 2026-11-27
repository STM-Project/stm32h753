"""Ensure generated files are changed only within CubeMX-preserved sections."""
from pathlib import Path
import hashlib,json,re,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
pattern=re.compile(r'(/\* USER CODE BEGIN ([^\n]*?) *\*/).*?(/\* USER CODE END \2 *\*/)',re.S)
manifest=json.loads((root/'tests/fixtures/network_generated_manifest.json').read_text())
for file,digest in manifest.items():
 source=(root/file).read_text()
 stripped=pattern.sub(lambda m:m[1]+'\n'+m[3],source)
 assert hashlib.sha256(stripped.encode()).hexdigest()==digest,file+' edited outside USER CODE'
eth=(root/'LWIP/Target/ethernetif.c').read_text()
assert '#if 0' in pattern.search(eth,eth.index('/* USER CODE BEGIN 0 */')).group(0)
assert 'network_ethernetif.inc' not in eth
assert not (root/'Application/NETWORK/network_ethernetif.inc').exists()
adapter=next(m.group(0) for m in pattern.finditer(eth) if m[2]=='8')
assert 'static int low_level_init' in adapter
assert 'HAL_ETH_RxLinkCallback' in adapter
assert 'SCB_InvalidateDCache_by_Addr' in adapter
# No nested markers: CubeMX must preserve the complete implementation.
assert adapter.count('/* USER CODE BEGIN')==1
assert adapter.count('/* USER CODE END')==1
assert eth.count('#if 0')==1
assert '#define MX_MBEDTLS_Init NetworkCubeMX_TLS_Placeholder' in (root/'MBEDTLS/App/mbedtls.c').read_text()
assert 'if ( !osKernelRunning () || tlsReady )\n\t\treturn;' in (root/'Application/NETWORK/network_tls.c').read_text()
assert '#define mbedtls_hardware_poll NetworkCubeMX_RNG_Placeholder' in (root/'MBEDTLS/Target/hardware_rng.c').read_text()
assert 'NetworkPrepareDmaMemory();' in (root/'LWIP/App/lwip.c').read_text()
ioc=(root/'STM32h753bit6.ioc').read_text().splitlines()
assert 'ProjectManager.KeepUserCode=true' in ioc
assert 'ProjectManager.DeletePrevious=false' in ioc
for config in ET.parse(root/'.cproject').findall('.//configuration'):
 assert 'h753_update_key.mk prepare-key' in config.get('prebuildStep','')
 script=next(opt for opt in config.iter('option') if opt.get('name')=='Linker Script (-T)')
 assert script.get('value','').endswith('/Application/STM32H753_application.ld}')
sd=(root/'FATFS/Target/sd_diskio.c').read_text()
read=next(m.group(0) for m in pattern.finditer(sd) if m[2]=='beforeReadSection')
assert 'DRESULT SD_read(' in read and '#if 0' in read
tls=(root/'MBEDTLS/App/mbedtls_config.h').read_text()
assert any('#define MBEDTLS_PLATFORM_TIME_ALT' in m.group(0) for m in pattern.finditer(tls))
main=(root/'Core/Src/main.c').read_text()
assert any('SCB->VTOR = (uint32_t) __h753_app_vectors' in m.group(0) for m in pattern.finditer(main))
print('CubeMX: generated code unchanged outside USER CODE; application TLS/RNG/DMA hooks preserved: PASS')
