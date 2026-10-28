from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import hashlib,json,subprocess,tempfile,threading,unittest
ROOT=Path(__file__).resolve().parents[1];EXE=ROOT.parent/'SNUWCANVAS.exe'
class ConsentRegistrationTests(unittest.TestCase):
 def test_account_me_switch_and_bounded_428_recovery(self):
  calls=[];registered=set();retry_count=0
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def reply(self,data,status=200):
    raw=json.dumps(data).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
   def principal(self):return self.headers.get('Authorization','')
   def do_GET(self):
    nonlocal retry_count
    calls.append(('GET',self.path,self.principal()))
    if self.path=='/api/version':return self.reply({'min_client_version':'v2.0.0'})
    if self.path=='/api/legal':return self.reply({'version':'consent-test','text':'Consent transport fixture','digest':hashlib.sha256(b'Consent transport fixture').hexdigest()})
    if self.path=='/api/retry':
     retry_count+=1
     if retry_count==1:registered.discard(self.principal())
    if self.path=='/api/always428' or self.principal() not in registered:return self.reply({'detail':'Privacy consent registration required'},428)
    return self.reply({'ok':True})
   def do_POST(self):
    body=json.loads(self.rfile.read(int(self.headers['Content-Length'])));calls.append(('POST',self.path,self.principal()))
    assert self.path=='/api/devices/register';assert body['required_agreed'];assert not body['hardware_agreed'];assert 'hardware' not in body
    assert self.headers['X-Client-Id']==body['client_id'];assert self.principal().startswith('Bearer ')
    registered.add(self.principal());self.reply({'registered':True})
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    result=subprocess.run([str(EXE),'--consent-network-test',folder,f'http://127.0.0.1:{server.server_port}'],timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
    report=json.loads((Path(folder)/'consent-network-result.json').read_text(encoding='utf-8'));self.assertEqual(result.returncode,0,report);self.assertTrue(report['bounded'])
   posts=[row for row in calls if row[0]=='POST'];self.assertEqual(len(posts),4);self.assertNotEqual(posts[0][2],posts[1][2])
   self.assertEqual(sum(path=='/api/always428' for method,path,who in calls),2)
   self.assertEqual(sum(path=='/api/retry' for method,path,who in calls),2)
   self.assertEqual(sum(path=='/api/auth/me' for method,path,who in calls),3)
  finally:server.shutdown();server.server_close();thread.join(timeout=2)
