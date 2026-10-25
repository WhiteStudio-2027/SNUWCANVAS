"""Exercise the compiled Windows caption transport with a local fake API only."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import subprocess
import tempfile
import threading
import time
import unittest
from urllib.parse import urlsplit, parse_qs

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT.parent / 'SNUWCANVAS.exe'


@unittest.skipUnless(EXE.exists(), 'Build native client first')
class CaptionClientTests(unittest.TestCase):
    def test_account_caption_uses_generation_api_without_caption_key(self):
        from PIL import Image
        calls=[]
        class Handler(BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def reply(self,value):
                body=json.dumps(value).encode();self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
            def do_GET(self):
                calls.append((self.path,dict(self.headers)))
                self.reply({'api_version':'0.3.0','models':[{'name':'vision:model/8b'}]})
            def do_POST(self):
                self.rfile.read(int(self.headers['Content-Length']));calls.append((self.path,dict(self.headers)))
                self.reply({'api_version':'0.3.0','text':'solo, blue hair','negative_prompt':'duplicate subject, blurry'})
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        try:
            with tempfile.TemporaryDirectory(dir=ROOT/'.build') as name:
                folder=Path(name);Image.new('RGB',(16,24),'blue').save(folder/'input.png')
                result=subprocess.run([str(EXE),'--caption-test',name,f'http://127.0.0.1:{server.server_port}','account'],timeout=15,creationflags=subprocess.CREATE_NO_WINDOW)
                report=json.loads((folder/'caption-result.json').read_text(encoding='utf-8'))
                self.assertEqual(result.returncode,0,report)
                self.assertEqual(calls[0][0],'/api/caption/models')
                self.assertEqual(urlsplit(calls[1][0]).path,'/api/caption/captions')
                self.assertTrue(all(row[1].get('Authorization')=='Bearer '+'a'*32 for row in calls))
                self.assertTrue(all('X-API-Key' not in row[1] for row in calls))
                self.assertTrue(all(row[1].get('X-Content-Mode')=='hardcore' for row in calls))
        finally:server.shutdown();server.server_close();thread.join(timeout=2)

    def test_native_raw_image_model_key_response_and_cancel(self):
        from PIL import Image
        calls = []
        delayed = False

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def reply(self, data):
                raw = json.dumps(data).encode()
                self.send_response(200)
                self.send_header('Content-Length', str(len(raw)))
                self.end_headers()
                try:
                    self.wfile.write(raw)
                except OSError:
                    pass

            def do_GET(self):
                calls.append(('GET', self.path, self.headers.get('X-API-Key'), b''))
                self.reply({'api_version': '0.3.0', 'models': [{'name': 'vision:model/8b', 'vision': True}]})

            def do_POST(self):
                body = self.rfile.read(int(self.headers['Content-Length']))
                calls.append(('POST', self.path, self.headers.get('X-API-Key'), body))
                if delayed:
                    time.sleep(3)
                self.reply({'api_version': '0.3.0', 'text': 'solo, blue hair', 'caption': 'A person with blue hair.',
                            'tags': ['solo', 'blue hair'], 'negative_prompt': 'duplicate subject, blurry'})

        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            for mode in ('normal', 'cancel'):
                delayed = mode == 'cancel'
                with tempfile.TemporaryDirectory(dir=ROOT / '.build') as folder:
                    path = Path(folder)
                    Image.new('RGB', (16, 24), 'blue').save(path / 'input.png')
                    proc = subprocess.run([str(EXE), '--caption-test', str(path),
                        f'http://127.0.0.1:{server.server_port}', mode], timeout=10,
                        creationflags=subprocess.CREATE_NO_WINDOW)
                    report = json.loads((path / 'caption-result.json').read_text(encoding='utf-8'))
                    self.assertEqual(proc.returncode, 0, report)
                    self.assertFalse(report['gui_opened'])
                    self.assertEqual(report['cancelled'], delayed)
            self.assertEqual(len(calls), 4)
            for method, url, key, body in calls:
                self.assertEqual(key, 'caption-test-secret')
                if method == 'POST':
                    self.assertTrue(body.startswith(b'\x89PNG'))
                    self.assertEqual(urlsplit(url).path, '/v1/captions')
                    self.assertEqual(parse_qs(urlsplit(url).query),
                        {'model': ['vision:model/8b'], 'format': ['tags'], 'mode': ['prompt'], 'pipeline': ['sdxl']})
                else:
                    self.assertEqual(url, '/v1/models')
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=2)
