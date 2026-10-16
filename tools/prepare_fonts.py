"""Fetch Google Fonts and create static faces for Tk/Windows GDI packaging."""
from pathlib import Path
import hashlib
import io
import json
import sys
import requests

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.dev-deps'/'fonttools'))
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

def main():
    target=ROOT/'assets'/'fonts';target.mkdir(parents=True,exist_ok=True)
    session=requests.Session()
    response=session.get('https://api.github.com/repos/google/fonts/commits/main',timeout=30)
    response.raise_for_status();revision=response.json()['sha']
    manifest=[]
    for folder,family,filename,weights in [
        ('notosanskr','NotoSansKR','NotoSansKR[wght].ttf',(400,700)),
        ('googlesanscode','GoogleSansCode','GoogleSansCode[wght].ttf',(400,))]:
        base=f'https://raw.githubusercontent.com/google/fonts/{revision}/ofl/{folder}/'
        response=session.get(base+filename,timeout=90);response.raise_for_status()
        source=response.content
        for weight in weights:
            font=instantiateVariableFont(TTFont(io.BytesIO(source)),{'wght':weight},inplace=True,updateFontNames=True)
            output=target/f'{family}-{"Bold" if weight==700 else "Regular"}.ttf'
            font.save(output);font.close()
            manifest.append(dict(file=output.name,source=base+filename,weight=weight,sha256=hashlib.sha256(output.read_bytes()).hexdigest()))
            print(output.name,output.stat().st_size)
        response=session.get(base+'OFL.txt',timeout=30);response.raise_for_status()
        (target/f'{family}-OFL.txt').write_bytes(response.content)
    (target/'sources.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

if __name__=='__main__':main()
