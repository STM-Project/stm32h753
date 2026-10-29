#!/usr/bin/env python3
"""Build the LCD's /nowa thermometer frames from the unmodified HTTP SVG.
Offline dependencies: cairosvg, lz4, Pillow, lxml. Firmware has no new library.
Matches panel.js colour ramp and jquery.thermometer.js geometry/data-* attributes.
The widget's rgb() blend fallback makes its two darkColours black; retain that
behaviour. Native LCD range labels are drawn separately.
"""
import sys,re,io,copy
import cairosvg,lz4.block
from lxml import etree as E
from PIL import Image
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'Application/LPC2148/web/svg/thermo-bottom.svg').read_text()
s=s.replace('<svg','<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:cc="http://creativecommons.org/ns#" xmlns:dc="http://purl.org/dc/elements/1.1/"',1)
root=E.fromstring(s.encode());root.set('viewBox','300 150 240 600');root.set('width','112');root.set('height','280')
for e in root.iter():
 if E.QName(e).localname=='text':e.getparent().remove(e)
frames=[];last=None;offsets=[0]
for i in range(101):
 r=copy.deepcopy(root);p=i/100
 v={'colour':f'rgb({int(255*p+.5)},{int(128*(1-p)+.5)},{int(255*(1-p)+.5)})','darkColour':'#000000','veryDarkColour':'#000000','tickColour':'#000000','textColour':'#000000','liquidY':346-p*321,'neckPosition':p*323+30,'boxPosition':573-(p*323+30)}
 for e in r.iter():
  for key,val in list(e.attrib.items()):
   if key.startswith('data-'):
    for k,x in v.items():val=val.replace('%%'+k+'%%',str(x))
    e.set(key[5:],val)
 b=cairosvg.svg2png(bytestring=E.tostring(r),background_color='white');im=Image.open(io.BytesIO(b)).convert('RGB')
 rgb=im.tobytes();raw=bytearray()
 for red,green,blue in zip(rgb[0::3],rgb[1::3],rgb[2::3]):
  v=((red>>3)<<11)|((green>>2)<<5)|(blue>>3);raw.extend((v&255,v>>8))
 delta=bytes(raw) if i%10==0 else bytes((a-b)&255 for a,b in zip(raw,last))
 compressed=lz4.block.compress(delta,store_size=False,mode='high_compression',compression=12)
 frames.append(compressed);offsets.append(offsets[-1]+len(compressed));last=raw
packed=b''.join(frames)
out=ROOT/'Application/LCD/Screens/lpc_thermometer_frames.inc'
lines=['/* Generated from web/svg/thermo-bottom.svg by tools/generate_lcd_thermometers.py.',
       ' * 101 RGB565 levels, LZ4 blocks; key frame every 10 levels. Do not edit. */',
       'static const uint32_t thermoOffsets[102] = {'+','.join(map(str,offsets))+'};',
       'static const uint8_t thermoFrames[] = {']
lines += [','.join('0x%02x'%x for x in packed[n:n+24])+',' for n in range(0,len(packed),24)]
lines+=['};','']
out.write_text('\n'.join(lines))
print(f'{out}: {len(packed)} compressed bytes, 101 SVG levels')
