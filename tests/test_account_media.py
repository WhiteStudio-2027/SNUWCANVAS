import base64,hashlib,json,subprocess,tempfile,threading,time,unittest
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class AccountMediaTests(unittest.TestCase):
 def test_native_private_pictures_and_typing(self):
  pixels=base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aD1sAAAAASUVORK5CYII=')
  digest=hashlib.sha256(pixels).hexdigest();state={'version':0,'avatar':True};calls=[];errors=[]
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def send(self,code,body,media='application/json'):
    raw=json.dumps(body).encode() if media=='application/json' else body
    self.send_response(code);self.send_header('Content-Length',str(len(raw)));self.send_header('Content-Type',media);self.end_headers();self.wfile.write(raw)
   def do_GET(self):
    if self.headers.get('X-API-Key')!='profile-test-key':errors.append('credentials')
    if self.path=='/api/profile-media':
     current={'sha256':digest,'url':'https://evil.invalid/never-fetch'}
     self.send(200,{'account_id':'media-owner','profile_version':format(state['version'],'064x'),'media':{'avatar':{'current':current if state['avatar'] else None,'can_undo':not state['avatar']},'banner':{'current':current}}})
    elif self.path in ('/api/profile-media/avatar','/api/profile-media/banner'):self.send(200,pixels,'image/png')
    else:errors.append(self.path);self.send(404,{})
   def mutate(self,action):
    raw=self.rfile.read(int(self.headers.get('Content-Length',0)));calls.append(action)
    if action=='upload':
     if b'expected_version' not in raw or format(state['version'],'064x').encode() not in raw:errors.append('upload version')
     time.sleep(.12)
    elif json.loads(raw)['expected_version']!=format(state['version'],'064x'):errors.append('stale version')
    state.update(version=state['version']+1,avatar=action!='clear')
    self.send(200,{'account_id':'media-owner','profile_version':format(state['version'],'064x')})
   def do_DELETE(self):self.mutate('clear')
   def do_POST(self):self.mutate('undo' if self.path.endswith('/undo') else 'upload')
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--account-media-test',folder,'http://127.0.0.1:'+str(server.server_port)],cwd=folder,timeout=40,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    report=json.loads((Path(folder)/'account-media-result.json').read_text(encoding='utf-8'))
    self.assertEqual(process.returncode,0,(report,calls,errors));self.assertEqual(errors,[]);self.assertEqual(calls,['clear','undo','upload'])
    for name in ('ok','decode','private_cache','undo','typing','isolation'):self.assertTrue(report[name],name)
    self.assertEqual(report['viewports'],3)
  finally:server.shutdown();server.server_close();thread.join()
