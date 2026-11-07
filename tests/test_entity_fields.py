import json,subprocess,tempfile,threading,unittest
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
ROOT=Path(__file__).resolve().parents[1]
class EntityFieldsTests(unittest.TestCase):
 def test_native_create_edit_prompt_only_insert_and_undo(self):
  requests=[];row={'id':'c'*32,'name':'Named Mug','kind':'object','description':'ceramic object','pipeline':'sdxl','auto_train':0,'lora':'','revision':0,'edit_version':'a'*64,'conditioning':'prompt','prompt_slots':{'appearance':'red ceramic mug','lighting':'warm light'}}
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def reply(self,value):
    data=json.dumps(value).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
   def do_POST(self):
    body=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(('POST',self.path,body,self.headers.get('X-API-Key')));self.reply(row)
   def do_GET(self):requests.append(('GET',self.path,{},self.headers.get('X-API-Key')));self.reply({**row,'references':[],'training':[],'training_setup':{'ready':False,'reason':'prompt_only','minimum_images':0},'dataset_count':0})
   def do_PATCH(self):
    body=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(('PATCH',self.path,body,self.headers.get('X-API-Key')));row.update(name=body['name'],description=body['description'],prompt_slots=body['prompt_slots'],edit_version='b'*64);self.reply(row)
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True;worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--entity-fields-test',folder,f'http://127.0.0.1:{server.server_port}'],timeout=45,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    report=json.loads((Path(folder)/'entity-fields-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
    for key in ('ok','prompt_only','identity_preserved','edit','insert_undo'):self.assertTrue(report[key],key)
   self.assertEqual([r[0] for r in requests],['POST','GET','PATCH'])
   self.assertTrue(all(r[3]=='entity-test-key' for r in requests));created=requests[0][2];edited=requests[2][2]
   self.assertEqual(created['kind'],'object');self.assertFalse(created['auto_train']);self.assertEqual(created['prompt_slots'],{'appearance':'red ceramic mug','lighting':'warm light'})
   self.assertEqual(edited['expected_version'],'a'*64);self.assertEqual(edited['prompt_slots'],{'appearance':'blue ceramic mug','lighting':'warm light'})
  finally:server.shutdown();server.server_close();worker.join(timeout=3)
