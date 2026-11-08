import json,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class AuditionTests(unittest.TestCase):
 def test_300_candidates_resume_ratings_and_scope(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--audition-test',folder],timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((Path(folder)/'audition-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
   self.assertEqual(report['tasks'],300)
   for key in ('ok','resume','uncertain_not_replayed','ratings','isolation','snapshot'):self.assertTrue(report[key],key)

 def test_resume_uses_existing_job_without_another_post(self):
  import io,threading
  from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
  from PIL import Image
  raw=io.BytesIO();Image.new('RGB',(32,32),'blue').save(raw,'PNG');image=raw.getvalue();calls=[];job='c'*32
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def send(self,value,kind='application/json'):
    body=json.dumps(value).encode() if kind=='application/json' else value
    self.send_response(200);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
   def do_POST(self):
    calls.append(('POST',self.path));self.rfile.read(int(self.headers.get('Content-Length',0)));self.send({'id':job})
   def do_GET(self):
    calls.append(('GET',self.path))
    if self.path.endswith('/events'):
     payload={'id':job,'state':'completed','count':1,'completed':1,'results':[{'image_url':'/image','seed':123}]}
     self.send((json.dumps(payload)+'\n').encode(),'application/x-ndjson')
    elif self.path=='/image':self.send(image,'image/png')
    elif self.path=='/jobs/'+job:self.send({'id':job})
    else:self.send_error(404)
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--audition-network-test',folder,'http://127.0.0.1:'+str(server.server_port)],timeout=25,cwd=folder,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    report=json.loads((Path(folder)/'audition-network-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
    self.assertEqual(calls.count(('POST','/jobs')),1);self.assertEqual(calls.count(('GET','/jobs/'+job)),1);self.assertEqual(report['terminal'],2)
  finally:server.shutdown();server.server_close();thread.join()

 def test_responsive_pair_pixels_scope_and_overall_progress(self):
  from PIL import Image
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   folder=Path(folder);Image.new('RGB',(48,72),'#bb3344').save(folder/'a.png');Image.new('RGB',(64,48),'#2255bb').save(folder/'b.png')
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--audition-comparison-test',str(folder)],timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((folder/'audition-comparison-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
   self.assertEqual(report['layouts'],8)
   for key in ('gpu_pixels','async_decode','scope','progress'):self.assertTrue(report[key],key)
