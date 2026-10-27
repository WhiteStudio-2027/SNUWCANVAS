import hashlib,hmac,json,subprocess,tempfile,threading,unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];EXE=ROOT.parent/'SNUWCANVAS.exe'
class GateClientTests(unittest.TestCase):
 def test_native_proof_matches_server_and_never_sends_password(self):
  salt=bytes(range(16));key=hashlib.pbkdf2_hmac('sha256',b'test-shared-password-1234',salt,600000);calls=[];identity='1'*32;nonce='2'*64
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def reply(self,data,status=200):
    raw=json.dumps(data).encode();self.send_response(status);self.send_header('Content-Length',str(len(raw)));self.send_header('Content-Type','application/json');self.end_headers();self.wfile.write(raw)
   def do_GET(self):
    calls.append((self.path,dict(self.headers),b''))
    if self.path.endswith('/challenge'):return self.reply({'required':True,'id':identity,'nonce':nonce,'salt':salt.hex(),'iterations':600000})
    self.reply({'ok':self.headers.get('X-Server-Access')=='t'*43})
   def do_POST(self):
    raw=self.rfile.read(int(self.headers['Content-Length']));calls.append((self.path,dict(self.headers),raw));body=json.loads(raw)
    expected=hmac.new(key,('snuw-main-access-v1:'+identity+':'+nonce).encode(),hashlib.sha256).hexdigest()
    if body['proof']!=expected:return self.reply({'detail':'invalid proof'},403)
    self.reply({'token':'t'*43,'expires_in':43200})
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    for restart in range(2):
     result=subprocess.run([str(EXE),'--server-access-test',folder,f'http://127.0.0.1:{server.server_port}'],timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
     status=json.loads((Path(folder)/'server-access-result.json').read_text());self.assertEqual(result.returncode,0,status);self.assertTrue(status['password_not_transmitted']);self.assertEqual(status['restored'],bool(restart));self.assertNotIn('test-shared-password',str(calls))
    self.assertNotIn(b'test-shared-password',(Path(folder)/'server-access.bin').read_bytes())
    self.assertEqual(sum(path.endswith('/unlock') for path,_,_ in calls),2)
  finally:server.shutdown();server.server_close();thread.join(timeout=2)
