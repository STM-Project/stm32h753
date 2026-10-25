"""Ensure generated files are changed only within CubeMX-preserved sections."""
from pathlib import Path
import hashlib,json,re
root=Path(__file__).resolve().parents[1]
pattern=re.compile(r'(/\* USER CODE BEGIN ([^\n]*?) \*/).*?(/\* USER CODE END \2 \*/)',re.S)
manifest=json.loads((root/'tests/fixtures/network_generated_manifest.json').read_text())
for file,digest in manifest.items():
 source=(root/file).read_text()
 stripped=pattern.sub(lambda m:m[1]+'\n'+m[3],source)
 assert hashlib.sha256(stripped.encode()).hexdigest()==digest,file+' edited outside USER CODE'
eth=(root/'LWIP/Target/ethernetif.c').read_text()
assert '#if 0' in pattern.search(eth,eth.index('/* USER CODE BEGIN 0 */')).group(0)
assert '#include "network_ethernetif.inc"' in eth
assert eth.count('#if 0')==1
assert '#define MX_MBEDTLS_Init NetworkCubeMX_TLS_Placeholder' in (root/'MBEDTLS/App/mbedtls.c').read_text()
assert 'if(!osKernelRunning() || tlsReady)return;' in (root/'Application/NETWORK/network_tls.c').read_text()
assert '#define mbedtls_hardware_poll NetworkCubeMX_RNG_Placeholder' in (root/'MBEDTLS/Target/hardware_rng.c').read_text()
assert 'NetworkPrepareDmaMemory();' in (root/'LWIP/App/lwip.c').read_text()
print('CubeMX: generated code unchanged outside USER CODE; application TLS/RNG/DMA hooks preserved: PASS')
