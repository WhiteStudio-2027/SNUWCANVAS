import json,subprocess,tempfile,threading,time,unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

class StartupWorkerTests(unittest.TestCase):
 def test_slow_network_keeps_messages_cancellable_and_retains_login(self):
  calls=[]
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def do_GET(self):
    calls.append(self.path);status=200
    if self.path=='/slow':time.sleep(0.8);value={'ok':True}
    elif self.path=='/api/auth/me':value={'account_id':'account-a','role':'safe'}
    elif self.path=='/api/usage':status=503;value={'public_message':'사용량 조회 실패'}
    else:status=404;value={}
    body=json.dumps(value).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(body)));self.end_headers()
    try:self.wfile.write(body)
    except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError):pass
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
    result=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--startup-worker-test',folder,'http://127.0.0.1:'+str(server.server_port)],cwd=folder,timeout=25,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));report=json.loads((Path(folder)/'startup-worker-result.json').read_text(encoding='utf-8'));self.assertEqual(result.returncode,0,report)
    self.assertGreaterEqual(report['message_beats'],5)
    for key in ('ok','network_cancel','consent_cancel','session_preserved','usage_optional'):self.assertTrue(report[key],key)
    self.assertEqual(calls.count('/api/auth/me'),1);self.assertEqual(calls.count('/api/usage'),1)
  finally:server.shutdown();server.server_close();thread.join()
