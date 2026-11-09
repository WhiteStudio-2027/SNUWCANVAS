import io,json,subprocess,tempfile,threading,unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]

class AuditionPromotionTests(unittest.TestCase):
 def test_selected_reference_integrity_and_explicit_non_training_upload(self):
  raw=io.BytesIO();Image.new('RGB',(48,72),'#bb3344').save(raw,'PNG');image=raw.getvalue();calls=[];bodies=[];profile='c'*32
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def send(self,value):
    body=json.dumps(value).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
   def do_GET(self):
    calls.append(('GET',self.path));self.send({'id':profile,'name':'Target','kind':'character','references':[],'auto_train':True,'training_setup':{'ready':True}})
   def do_POST(self):
    calls.append(('POST',self.path));bodies.append(self.rfile.read(int(self.headers.get('Content-Length',0))));self.send({'id':'e'*32})
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    folder=Path(folder);(folder/'fixture.png').write_bytes(image)
    process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--audition-promotion-test',str(folder),'http://127.0.0.1:'+str(server.server_port)],cwd=folder,timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    report=json.loads((folder/'audition-promotion-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
    for key in ('ok','scope_mode_server','selected_only','file_integrity','provenance','explicit_upload'):self.assertTrue(report[key],key)
    posts=[path for method,path in calls if method=='POST'];self.assertEqual(posts,['/learning/profiles/'+profile+'/references'])
    self.assertEqual(len(bodies),1);self.assertIn(image,bodies[0]);self.assertIn(b'name="defer_training"\r\n\r\ntrue',bodies[0]);self.assertNotIn(b'changed',bodies[0])
  finally:server.shutdown();server.server_close();thread.join()
