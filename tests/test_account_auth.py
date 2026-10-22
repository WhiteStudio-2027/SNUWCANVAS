"""Real hidden native login/signup transport against a synthetic local API only."""
import http.server
import json
from pathlib import Path
import subprocess
import tempfile
import threading
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT.parent/'SNUWCANVAS.exe'


class Handler(http.server.BaseHTTPRequestHandler):
    calls=[]
    def log_message(self,*args):pass
    def do_POST(self):
        body=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        type(self).calls.append((self.path,body,bool(self.headers.get('X-Client-Id'))))
        if body.get('email')=='blocked@example.invalid':
            data=b'<html>category blocked</html>';media='text/html'
        elif body.get('email')=='malformed@example.invalid':
            data=b'client security blocked';media='application/json'
        elif body.get('email')=='denied@example.invalid':
            data=b'{"detail":"private technical trace"}';media='application/json'
        else:
            assert len(body.get('password',''))>=4
            if self.path.endswith('/signup'):
                assert body['nickname']=='Test User'
                assert body['required_agreed'] is True
            data=json.dumps({'access_token':'a'*40,'refresh_token':'r'*60,'expires_in':900,'account_id':'synthetic'}).encode()
            media='application/json'
        self.send_response(401 if body.get('email')=='denied@example.invalid' else 200);self.send_header('Content-Type',media)
        self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)


@unittest.skipUnless(EXE.exists(),'Build client first')
class AccountAuthTests(unittest.TestCase):
    def test_login_signup_and_blocked_response(self):
        Handler.calls=[]
        with http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler) as server:
            thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
            try:
                with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
                    result=subprocess.run([str(EXE),'--account-auth-test',folder,
                        f'http://127.0.0.1:{server.server_port}'],timeout=25,
                        creationflags=subprocess.CREATE_NO_WINDOW)
                    state=json.loads((Path(folder)/'account-auth-result.json').read_text(encoding='utf-8'))
                    self.assertEqual(result.returncode,0,state)
                    self.assertTrue(state['login']);self.assertTrue(state['signup'])
                    self.assertTrue(state['safe_error']);self.assertFalse(state['gui_opened'])
                self.assertEqual([c[0] for c in Handler.calls],['/api/auth/login','/api/auth/signup','/api/auth/login','/api/auth/login','/api/auth/login'])
                self.assertTrue(all(c[2] for c in Handler.calls))
            finally:server.shutdown();thread.join()
