"""Exercise native WinHTTP/multipart/image saving against a local fake API. No UI."""
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from email.parser import BytesParser
from email.policy import default
import io
import json
import subprocess
import tempfile
import threading
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]

def main():
    png=io.BytesIO();Image.new('RGB',(32,48),'#68a86c').save(png,format='PNG')
    calls=[];errors=[]
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*_):pass
        def send(self,value,kind='application/json'):
            body=json.dumps(value).encode() if kind=='application/json' else value
            self.send_response(200);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
        def do_POST(self):
            try:
                assert self.headers.get('X-API-Key')=='native-test-key'
                body=self.rfile.read(int(self.headers.get('Content-Length','0')))
                calls.append(self.path)
                if self.path=='/jobs':
                    message=BytesParser(policy=default).parsebytes(('Content-Type: '+self.headers['Content-Type']+'\r\nMIME-Version: 1.0\r\n\r\n').encode()+body)
                    fields={p.get_param('name',header='content-disposition'):p.get_payload(decode=True) for p in message.iter_parts()}
                    assert fields['count']==b'2'
                    controls=json.loads(fields['advanced_settings'])
                    assert controls['steps']==controls['hires_steps']==3
                    assert '한글' in fields['prompt'].decode()
                    assert fields['references']==png.getvalue()
                    self.send({'id':'generation-test'})
                elif self.path=='/sessions':self.send({'session_id':'edit-session'})
                elif self.path=='/sessions/edit-session/edit?background=true':
                    payload=json.loads(body);assert payload['text']=='옷을 초록색으로';assert len(payload['request_id'])==32
                    self.send({'id':'editing-test'})
                else:raise AssertionError(self.path)
            except Exception as exc:errors.append(str(exc));self.send_error(500)
        def stream(self,value):self.send((json.dumps(value)+"\n").encode(),"application/x-ndjson")
        def do_GET(self):
            calls.append(self.path)
            if self.headers.get('X-API-Key')!='native-test-key':errors.append('missing key')
            if self.path=='/jobs/generation-test/events':self.stream({'state':'completed','completed':2,'count':2,'results':[{'image_url':'/images/1','seed':1},{'image_url':'/images/2','seed':2}]})
            elif self.path=='/jobs/editing-test/events':self.stream({'state':'completed','completed':1,'count':1,'results':[{'image_url':'/images/edit','version':1}]})
            elif self.path=='/sessions/edit-session/history':self.send({'current_version':1,'versions':[{'version':1,'parent':0,'image_url':'/images/edit'}]})
            elif self.path.startswith('/images/'):self.send(png.getvalue(),'image/png')
            else:self.send_error(404)
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            path=Path(folder);(path/'reference.png').write_bytes(png.getvalue())
            result=subprocess.run([str(ROOT/'.build/SNUWCANVAS.exe'),'--api-test',folder,f'http://127.0.0.1:{server.server_port}'],timeout=60,creationflags=subprocess.CREATE_NO_WINDOW)
            report=json.loads((path/'native-api-result.json').read_text(encoding='utf-8'))
            assert result.returncode==0 and report['ok'],report
            assert not errors,errors
            assert calls.count('/jobs/generation-test/events')==1,calls
            assert calls.count('/jobs/editing-test/events')==1,calls
            assert '/jobs/generation-test' not in calls,calls
            images=list((path/'generated').glob('*.png'));assert len(images)==3,images
            for image in images:
                with Image.open(image) as data:assert data.size==(32,48)
                assert 'native-test-key' not in image.with_suffix('.json').read_text(encoding='utf-8')
            print(json.dumps(dict(report,multipart_validated=True,steps_preserved=True,metadata_key_excluded=True),ensure_ascii=False))
    finally:server.shutdown();server.server_close();thread.join(timeout=2)

if __name__=='__main__':main()
