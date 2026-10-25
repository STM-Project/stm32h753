"""Check generated/configuration boundaries of the H7 Ethernet port."""
from pathlib import Path
import re,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
cfg=dict(line.split('=',1) for line in (root/'STM32h753bit6.ioc').read_text().splitlines() if '=' in line)
assert cfg['LWIP.IP_ADDRESS']=='192.168.001.040'
assert cfg['LWIP.NETMASK_ADDRESS']=='255.255.255.000'
assert cfg['LWIP.GATEWAY_ADDRESS']=='192.168.001.001'
assert cfg['ETH.RxBuffLen']=='1536'
assert cfg['LWIP.LWIP_DHCP']=='0' and cfg['LWIP0.BSP.component']=='LAN8742'
for n,base,size in [(5,'0x30040000','MPU_REGION_SIZE_32KB'),(6,'0x30040000','MPU_REGION_SIZE_256B')]:
 suffix=f'_S-Cortex_Memory_Protection_Unit_Region{n}_Settings_S'
 assert cfg['CORTEX_M7.BaseAddress'+suffix]==base
 assert cfg['CORTEX_M7.Size'+suffix]==size
 assert cfg['CORTEX_M7.IsCacheable'+suffix]=='MPU_ACCESS_NOT_CACHEABLE'
# Explicit user boundary: USART6/ESP32 must retain the existing mapping/mode.
assert cfg['Dma.USART6_RX.0.Instance']=='DMA1_Stream0'
assert cfg['Dma.USART6_RX.0.Mode']=='DMA_CIRCULAR'
assert cfg['Dma.USART6_RX.0.Priority']=='DMA_PRIORITY_LOW'
assert cfg['Dma.USART6_TX.1.Instance']=='DMA1_Stream1'
assert cfg['Dma.USART6_TX.1.Mode']=='DMA_NORMAL'
assert cfg['NVIC.USART6_IRQn'].split('\\:')[1]=='5'
assert 'configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5' in (root/'Core/Inc/FreeRTOSConfig.h').read_text()
for irq in ['SDMMC1','MDMA']:
 assert cfg[f'NVIC.{irq}_IRQn'].split('\\:')[1]=='5'
for kind in ['IP','Pin']:
 entries=[v for k,v in cfg.items() if re.fullmatch(r'Mcu\.'+kind+r'\d+',k)]
 count='Mcu.IPNb' if kind=='IP' else 'Mcu.PinsNb'
 assert len(entries)==int(cfg[count]) and len(set(entries))==len(entries)
for conf in ET.parse(root/'.cproject').findall('.//configuration'):
 names={e.get('name') for e in conf.findall('./sourceEntries/entry')}
 assert {'LWIP','MBEDTLS'}<=names
 values=[v.get('value','') for v in conf.findall('.//option/listOptionValue')]
 for inc in ['../LWIP/App','../LWIP/Target','../MBEDTLS/App','../Application/NETWORK']:
  assert inc in values
 assert 'MBEDTLS_CONFIG_FILE=<mbedtls_config.h>' in values
eth=(root/'Application/NETWORK/network_ethernetif.inc').read_text()
assert not re.search(r'^ETH_HandleTypeDef\s+heth\s*;',eth,re.M)
assert not re.search(r'^void HAL_ETH_MspInit\(',eth,re.M)
irq=(root/'Core/Src/stm32h7xx_it.c').read_text()
assert irq.count('void ETH_IRQHandler(void)')==1
assert '#include "eth.h"' not in irq
assert 'HAL_NVIC_SetPriority(ETH_IRQn,5,0)' in (root/'Application/NETWORK/network_eth_hardware.c').read_text()
assert 'GETVAL_ptr' not in (root/'Application/NETWORK/network_http.c').read_text()
ld=(root/'STM32H753BITX_FLASH.ld').read_text()
for text in ['.eth_dma 0x30040000','. = 0x60;','.eth_heap 0x30044000','.eth_rx_pool 0x30010000','ASSERT(SIZEOF(.eth_heap) <= 0x4000']:
 assert text in ld
# When a firmware build exists, verify actual placement rather than just script text.
mapfile=root/'Debug/cloud/firmware.map'
if mapfile.exists():
 text=mapfile.read_text()
 for name,address in [('eth_dma',0x30040000),('eth_heap',0x30044000),('eth_rx_pool',0x30010000),('d2_buffers',0x30000000)]:
  m=re.search(r'^\.'+name+r'\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)',text,re.M)
  assert m and int(m[1],16)==address
  if name=='eth_dma':assert int(m[2],16)<=256
  if name=='eth_heap':assert int(m[2],16)<=16384
print('Network config: F7 addresses/LAN8742, FP70 network MPU, linker/ARM map, IDE sources, unchanged USART6 and non-network DMA: PASS')
