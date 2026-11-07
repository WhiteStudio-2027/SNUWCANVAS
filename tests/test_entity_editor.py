from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import tempfile
import threading
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT.parent/'SNUWCANVAS.exe'


@unittest.skipUnless(EXE.exists(),'Build native client first')
class EntityEditorTests(unittest.TestCase):
    def test_native_completion_and_unavailable_service_never_send_prompt_prefix(self):
        calls=[]
        class Handler(BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def do_GET(self):
                calls.append((self.path,self.headers.get('X-API-Key')))
                payload=json.dumps({'suggestions':[{'id':'a'*32,'name':'하루','kind':'character','revision':0},
                                                   {'id':'b'*32,'name':'다른 인물','kind':'character','revision':0},
                                                   {'id':'c'*32,'name':'bad:name','kind':'character','revision':0}]},ensure_ascii=False).encode('utf-8')
                self.send_response(503 if self.path.startswith('/failure/') else 200)
                self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(payload)))
                self.end_headers();self.wfile.write(payload)
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
        worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start()
        try:
            for mode in ['success','failure']:
                with self.subTest(mode=mode),tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
                    command=[str(EXE),'--entity-editor-test',folder,f'http://127.0.0.1:{server.server_port}/{mode}',mode]
                    process=subprocess.run(command,timeout=20,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
                    result=json.loads((Path(folder)/'entity-editor-result.json').read_text('utf-8'))
                    self.assertEqual(process.returncode,0,result)
                    self.assertFalse(result['gui_opened']);self.assertFalse(result['private_prefix_sent'])
            self.assertEqual(calls,[('/success/learning/profiles/suggestions?limit=100','entity-test-key'),
                                    ('/failure/learning/profiles/suggestions?limit=100','entity-test-key')])
        finally:server.shutdown();server.server_close();worker.join(timeout=2)
