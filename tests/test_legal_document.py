import hashlib,json,subprocess,tempfile,threading,unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

class LegalDocumentTests(unittest.TestCase):
 def test_native_markdown_tables_layout_and_required_consent(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   result=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--legal-document-test',folder],cwd=folder,timeout=25,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((Path(folder)/'legal-document-result.json').read_text(encoding='utf-8'));self.assertEqual(result.returncode,0,report);self.assertEqual(report['layouts'],6)
   for key in ('ok','markdown','required_gate','literal_html','version_time'):self.assertTrue(report[key],key)

 def test_changed_registration_reloads_once_and_repeated_changes_stop(self):
  for always in (False,True):
   with self.subTest(always=always):
    calls=[];registered=[];text='Consent revision fixture'
    class Handler(BaseHTTPRequestHandler):
     def log_message(self,*args):pass
     def send(self,value,status=200):
      body=json.dumps(value).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
     def do_GET(self):
      calls.append(self.path)
      if self.path=='/api/version':self.send({'min_client_version':'v3.5.0'})
      elif self.path=='/api/legal':self.send({'version':'revision-1','text':text,'digest':hashlib.sha256(text.encode()).hexdigest()})
      else:self.send({},404)
     def do_POST(self):
      registered.append(json.loads(self.rfile.read(int(self.headers.get('Content-Length',0)))))
      if always or len(registered)==1:self.send({'detail':'Consent changed; review current document','public_message':'요청을 완료하지 못했습니다.'},409)
      else:self.send({'registered':True})
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
     with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
      command=[str(ROOT.parent/'SNUWCANVAS.exe'),'--consent-revision-test',folder,'http://127.0.0.1:'+str(server.server_port)]+(['always'] if always else [])
      result=subprocess.run(command,cwd=folder,timeout=25,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));report=json.loads((Path(folder)/'consent-revision-result.json').read_text(encoding='utf-8'));self.assertEqual(result.returncode,0,report)
      self.assertEqual(calls.count('/api/legal'),2);self.assertEqual(len(registered),2);self.assertTrue(all(row['required_agreed'] and not row['hardware_agreed'] for row in registered))
    finally:server.shutdown();server.server_close();thread.join()
