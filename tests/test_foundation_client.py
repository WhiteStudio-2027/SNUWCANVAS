from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
import hashlib
import json
import subprocess
import tempfile
import threading
import unittest
from uuid import UUID

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT.parent/'SNUWCANVAS.exe'

@unittest.skipUnless(EXE.exists(),'Build native client first')
class FoundationClientTests(unittest.TestCase):
    def test_dpapi_identity_and_consent_without_gui(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as name:
            folder=Path(name)
            result=subprocess.run([str(EXE),'--foundation-test',str(folder)],timeout=20,creationflags=subprocess.CREATE_NO_WINDOW)
            report=json.loads((folder/'foundation-test-result.json').read_text(encoding='utf-8'))
            self.assertEqual(result.returncode,0,report);self.assertTrue(report['ok']);self.assertFalse(report['gui_opened'])
            self.assertEqual(report['client_version'],'v2.0.0');self.assertFalse(report['optional_hardware'])
    def test_version_headers_registration_and_optional_collection(self):
        calls=[];text='Test-only privacy document';digest=hashlib.sha256(text.encode()).hexdigest()
        class Handler(BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def send_json(self,data):
                raw=json.dumps(data).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
            def do_GET(self):
                calls.append(('GET',self.path,dict(self.headers),None))
                if self.path=='/api/legal':return self.send_json({'version':'test-1','digest':digest,'text':text})
                if self.path=='/api/version':return self.send_json({'server_version':'v2.0.0','min_client_version':'v2.0.0','latest_client_version':'v2.0.0'})
                if self.path=='/protected':return self.send_json({'ok':True})
                self.send_error(404)
            def do_POST(self):
                body=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
                calls.append(('POST',self.path,dict(self.headers),body));self.send_json({'registered':True})
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        try:
            with tempfile.TemporaryDirectory(dir=ROOT/'.build') as name:
                folder=Path(name);base=f'http://127.0.0.1:{server.server_port}'
                result=subprocess.run([str(EXE),'--foundation-network-test',str(folder),base],timeout=20,creationflags=subprocess.CREATE_NO_WINDOW)
                report=json.loads((folder/'foundation-network-result.json').read_text(encoding='utf-8'))
                self.assertEqual(result.returncode,0,report);self.assertFalse(report['gui_opened'])
                posted=[row for row in calls if row[0]=='POST'];self.assertEqual(len(posted),1)
                _,route,headers,body=posted[0];self.assertEqual(route,'/api/devices/register')
                self.assertEqual(UUID(body['client_id']).version,4);self.assertEqual(headers['X-Client-Id'],body['client_id'])
                self.assertEqual(headers['X-Client-Version'],'v2.0.0');self.assertFalse(body['hardware_agreed']);self.assertNotIn('hardware',body)
                self.assertTrue(all(row[2].get('X-Client-Version')=='v2.0.0' for row in calls))
                control=[row for row in calls if row[1].startswith('/api/')]
                self.assertTrue(all(row[2].get('X-Content-Mode')=='standard' for row in control))
                self.assertEqual([row[2].get('X-Content-Mode') for row in calls if row[1]=='/protected'],['standard'])
        finally:server.shutdown();server.server_close();thread.join(timeout=2)
