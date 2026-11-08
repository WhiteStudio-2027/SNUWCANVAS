import json,threading,time,subprocess,tempfile,unittest
from pathlib import Path
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
ROOT=Path(__file__).resolve().parents[1]
class AccountAutosaveTests(unittest.TestCase):
 def test_real_native_debounce_typing_retry_restart_and_owner_guard(self):
  calls=[];errors=[];state={'version':0,'nickname':'Initial','bio':'','failed':False}
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def send(self,status,body):
    raw=json.dumps(body).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
   def profile(self):return {'account_id':'auto-owner','nickname':state['nickname'],'bio':state['bio'],'profile_version':format(state['version'],'064x')}
   def do_PATCH(self):
    body=json.loads(self.rfile.read(int(self.headers.get('Content-Length',0))));calls.append(body)
    if self.path!='/api/auth/me' or self.headers.get('X-API-Key')!='profile-test-key':errors.append('wrong endpoint or owner credentials')
    if body.get('expected_version')!=format(state['version'],'064x'):errors.append('stale write')
    if body['nickname']=='Alpha':time.sleep(.12)
    if body['nickname']=='FailOnce' and not state['failed']:
     state['failed']=True;self.send(503,{'public_message':'프로필 저장을 완료하지 못했습니다.'});return
    state.update(nickname=body['nickname'],bio=body['bio'],version=state['version']+1);self.send(200,self.profile())
   def do_GET(self):
    if self.path!='/api/auth/me':errors.append('wrong reload endpoint')
    state.update(nickname='Remote',bio='remote',version=state['version']+1);self.send(200,self.profile())
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--account-autosave-test',folder,'http://127.0.0.1:'+str(server.server_port)],timeout=35,cwd=folder,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    report=json.loads((Path(folder)/'account-autosave-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,(report,calls,errors))
    self.assertEqual(errors,[]);self.assertEqual([call['nickname'] for call in calls],['Alpha','Beta','FailOnce','FailOnce'])
    for key in ('ok','debounce','ime','typing_preserved','retry','draft_restart','backup','owner_binding','local_failure'):self.assertTrue(report[key],key)
    self.assertEqual(report['viewports'],3)
  finally:server.shutdown();server.server_close();thread.join()
