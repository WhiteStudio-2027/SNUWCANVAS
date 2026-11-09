import json,subprocess,tempfile,unittest,os,shutil
from contextlib import contextmanager
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
@contextmanager
def native_fixture():
 folder=Path(tempfile.mkdtemp(dir=ROOT/'.build')).resolve()
 try:yield str(folder)
 finally:
  resolved=folder.resolve();allowed=(ROOT/'.build').resolve()
  if resolved==allowed or not resolved.is_relative_to(allowed):raise RuntimeError('Unsafe test cleanup target')
  target=str(resolved)
  if os.name=='nt':target=chr(92)*2+'?'+chr(92)+target
  shutil.rmtree(target)
class ReferenceSheetTests(unittest.TestCase):
 def test_preparation_ui_review_provenance_and_late_responses(self):
  import threading,time,io
  from email.parser import BytesParser
  from email.policy import default
  from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
  calls=[];payloads=[];jobs=[];profile='b'*32
  image_file=io.BytesIO();Image.new('RGB',(32,48),'#4488bb').save(image_file,'PNG');image=image_file.getvalue()
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def send(self,value):
    body=json.dumps(value).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(body)));self.end_headers()
    try:self.wfile.write(body)
    except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError):pass
   def do_GET(self):
    calls.append(('GET',self.path))
    if self.path=='/sheet-image':
     self.send_response(200);self.send_header('Content-Type','image/png');self.send_header('Content-Length',str(len(image)));self.end_headers();self.wfile.write(image)
    elif self.path.startswith('/jobs/'):
     job=self.path.split('/')[2];value={'id':job,'state':'completed','count':1,'completed':1,'results':[{'image_url':'/sheet-image','entity_snapshot_verified':True,'seed':123}]}
     if self.path.endswith('/events'):
      body=(json.dumps(value)+'\n').encode();self.send_response(200);self.send_header('Content-Type','application/x-ndjson');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
     else:self.send(value)
    elif self.path.endswith('d'*32):self.send({'id':'d'*32,'name':'Style Test','kind':'style','content_version':'e'*64,'pipeline':'sdxl'})
    else:time.sleep(.12);self.send({'id':profile,'name':'Sheet Character','kind':'character','content_version':'c'*64,'pipeline':'sdxl'})
   def do_POST(self):
    calls.append(('POST',self.path));raw=self.rfile.read(int(self.headers.get('Content-Length',0)))
    if self.path=='/jobs':
     message=BytesParser(policy=default).parsebytes(('Content-Type: '+self.headers['Content-Type']+'\r\nMIME-Version: 1.0\r\n\r\n').encode()+raw)
     form={part.get_param('name',header='content-disposition'):part.get_payload(decode=True) for part in message.iter_parts()};jobs.append(form);self.send({'id':format(len(jobs),'032x')})
    else:
     value=json.loads(raw);payloads.append(value);self.send({'verified':True,'snapshot':value['previous']})
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with native_fixture() as folder:
    folder=Path(folder);Image.new('RGB',(32,48),'#4488bb').save(folder/'fixture.png')
    result=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--reference-sheet-ui-test',str(folder),'http://127.0.0.1:'+str(server.server_port)],cwd=folder,timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));report=json.loads((folder/'reference-sheet-ui-result.json').read_text(encoding='utf-8'));runs=[]
    if result.returncode:
     for file in folder.rglob('auditions/*.json'):
      data=json.loads(file.read_text(encoding='utf-8'));runs.extend([{key:task.get(key) for key in ('state','error','job','candidate')} for run in data.get('runs',[]) for task in run.get('tasks',[])])
    self.assertEqual(result.returncode,0,{'report':report,'calls':calls,'runs':runs});self.assertEqual(report['layouts'],4)
    shutil.copyfile(folder/'reference-sheet-ui.png',ROOT.parent/'docs/v3/reference-sheet-ui.png')
    for key in ('ok','ordinary_inputs_preserved','late_input_ignored','role_scope','inferred_provenance','four_job_sequence','five_camera_sequence','style_pair_sequence','no_auto_approval'):self.assertTrue(report[key],key)
    self.assertTrue(payloads);self.assertTrue(all(value['verify'] and value['profile_ids'][0]==profile for value in payloads));self.assertEqual(len(jobs),11);self.assertEqual(len({row['request_id'] for row in jobs}),11)
    for row,angle in zip(jobs,('front view','left side','right side','back view','close-up portrait','three-quarter view','front view, full body','high angle view','low angle view','front view','front view')):
     self.assertIn(angle,row['scene_prompt'].decode());self.assertEqual(row['references'],image);self.assertEqual(json.loads(row['profile_ids'])[0],profile);self.assertEqual(json.loads(row['entity_expectations'])['refs'][0]['content_version'],'c'*64)
    self.assertEqual(jobs[-2]['seed'],jobs[-1]['seed']);self.assertEqual(json.loads(jobs[-1]['profile_ids']),[profile,'d'*32]);self.assertEqual(json.loads(jobs[-1]['entity_expectations'])['refs'][1]['content_version'],'e'*64)
  finally:server.shutdown();server.server_close();thread.join()

 def test_four_angle_hypotheses_review_source_and_profile_versions(self):
  with native_fixture() as folder:
   folder=Path(folder);Image.new('RGB',(32,48),'#4488bb').save(folder/'fixture.png')
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--reference-sheet-test',str(folder)],cwd=folder,timeout=25,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));report=json.loads((folder/'reference-sheet-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report);self.assertEqual(report['angles'],4)
   for key in ('ok','back_unverified','approval_bound_to_hash','profile_pinned','source_integrity','restart','long_image_io'):self.assertTrue(report[key],key)
