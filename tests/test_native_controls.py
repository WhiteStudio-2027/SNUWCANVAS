"""Blocked submission/status/download cancellation against a fake local server."""
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import json
import subprocess
import tempfile
import threading
import time
import unittest
ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT/'.build/SNUWCANVAS.exe'
@unittest.skipUnless(EXE.exists(),'Build client first')
class ControlTests(unittest.TestCase):
    def test_stop_deadlines_download_failure_and_double_click(self):
        calls=[];markers={}
        class Handler(BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def mark(self,mode):
                if mode not in markers:return
                try:markers[mode].write_text('started',encoding='utf-8')
                except OSError:pass
            def reply(self,value,status=200,stream=False,hold=False):
                data=json.dumps(value).encode()+(b'\n' if stream else b'');self.send_response(status)
                if not stream:self.send_header('Content-Length',str(len(data)))
                self.end_headers()
                try:
                    self.wfile.write(data);self.wfile.flush()
                    if hold:time.sleep(5)
                except (BrokenPipeError,ConnectionResetError,OSError):pass
            def do_POST(self):
                self.rfile.read(int(self.headers.get('Content-Length',0)));calls.append(('POST',self.path))
                if self.path.endswith('/cancel'):self.reply({'ok':True,'cancelled':True});return
                if self.path=='/submit/jobs':self.mark('submit');time.sleep(5)
                self.reply({'id':'test-job'})
            def do_GET(self):
                calls.append(('GET',self.path));mode=self.path.split('/')[1]
                if '/images/' in self.path:
                    if mode=='download':self.mark('download');time.sleep(5)
                    self.reply({'detail':'Image not found'},404);return
                if mode=='poll':self.mark('poll');time.sleep(5)
                if mode in {'missing','download'}:self.reply({'state':'running','count':2,'completed':1,'results':[{'image_url':f'/{mode}/images/1'}]},stream=True)
                else:self.reply({'state':'running','count':2,'completed':0,'results':[]},stream=True,hold=mode=='timeout')
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        try:
            for mode in ('submit','poll','download','timeout','missing'):
                with self.subTest(mode=mode),tempfile.TemporaryDirectory(dir=ROOT/'.build') as name:
                    path=Path(name);markers[mode]=path/'network-started';base=f'http://127.0.0.1:{server.server_port}/{mode}'
                    result=subprocess.run([str(EXE),'--control-test',str(path),base,mode],timeout=15,creationflags=subprocess.CREATE_NO_WINDOW)
                    report=json.loads((path/'native-control-result.json').read_text(encoding='utf-8'))
                    self.assertEqual(result.returncode,0,report);self.assertTrue(report['confirmed']);self.assertFalse(report['gui_opened'])
                    self.assertEqual(sum(method=='POST' and url==f'/{mode}/jobs' for method,url in calls),1)
                    self.assertLessEqual(sum(method=='GET' and url.endswith('/events') and url.startswith(f'/{mode}/') for method,url in calls),1)
                    self.assertFalse(any(method=='GET' and url==f'/{mode}/jobs/test-job' for method,url in calls))
                    self.assertTrue(any(method=='POST' and url.startswith(f'/{mode}/client-requests/') for method,url in calls))
        finally:server.shutdown();server.server_close();thread.join(timeout=2)
