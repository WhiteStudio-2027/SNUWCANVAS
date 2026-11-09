import json,subprocess,tempfile,threading,time,unittest
from pathlib import Path
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
ROOT=Path(__file__).resolve().parents[1]
class ModelManagerTests(unittest.TestCase):
 def test_native_request_review_source_scope_and_layout(self):
  state={'row':None,'version':0};calls=[];errors=[]
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def send(self,body):
    raw=json.dumps(body).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
   def do_GET(self):
    if not self.path.startswith('/api/models/requests?'):errors.append(self.path)
    row=dict(state['row']) if state['row'] else None
    if row and 'poison=true' in self.path:row['owner']='other-owner'
    self.send({'requests':[row] if row else []})
   def do_POST(self):
    if self.headers.get('X-API-Key')!='model-test-key':errors.append('credentials')
    body=json.loads(self.rfile.read(int(self.headers.get('Content-Length',0))))
    if self.path=='/api/models/inspect':
     time.sleep(.06);self.send({'repo':body['repo'],'file':body['file'],'revision':'a'*40,'sha256':'b'*64,'bytes':100});return
    calls.append(self.path)
    if self.path=='/api/models/requests':
     time.sleep(.12)
     state['row']={'id':'c'*32,'owner':'model-owner','version':format(0,'032x'),'state':'requested','received':0,'verified_bytes':0,'spec':body,'error':''}
     if body['revision']!='a'*40 or body['bytes']!=100:errors.append('source metadata')
    else:
     if body['expected_version']!=format(state['version'],'032x'):errors.append('stale mutation')
     state['version']+=1;state['row']['version']=format(state['version'],'032x')
     if self.path.endswith('/approve'):
      if not body.get('license_reviewed') or not body.get('compatibility_reviewed'):errors.append('review flags')
      state['row']['state']='approved'
     elif self.path.endswith('/start'):state['row'].update(state='verified',received=100,verified_bytes=100)
     elif self.path.endswith('/activate'):state['row']['state']='active'
     else:errors.append(self.path)
    self.send(state['row'])
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--model-manager-test',folder,'http://127.0.0.1:'+str(server.server_port)],timeout=40,cwd=folder,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    report=json.loads((Path(folder)/'model-manager-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,(report,calls,errors));self.assertEqual(errors,[])
    self.assertEqual([path.rsplit('/',1)[-1] for path in calls],['requests','approve','start','activate'])
    for name in ('ok','inspect_guard','request','admin_actions','isolation','generation_unchanged'):self.assertTrue(report[name],name)
    self.assertEqual(report['layouts'],6)
  finally:server.shutdown();server.server_close();thread.join()
