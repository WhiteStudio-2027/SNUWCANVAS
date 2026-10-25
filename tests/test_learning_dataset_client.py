from email.parser import BytesParser
from email.policy import default
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import tempfile
import threading
import unittest
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT.parent/'SNUWCANVAS.exe'

@unittest.skipUnless(EXE.exists(),'Build native client first')
class LearningDatasetClientTests(unittest.TestCase):
 def test_real_native_hundred_uploads_mode_limit_and_delete(self):
  rows=[];calls=[];profile='a'*32
  class Handler(BaseHTTPRequestHandler):
   def log_message(self,*args):pass
   def reply(self,data):
    raw=json.dumps(data).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
   def do_GET(self):
    calls.append(('GET',self.path,dict(self.headers),None))
    if self.path.endswith(('/image','/thumbnail')):
     import io
     out=io.BytesIO();Image.new('RGB',(32,48),'navy').save(out,format='PNG');raw=out.getvalue()
     self.send_response(200);self.send_header('Content-Type','image/png');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw);return
    self.reply({'id':profile,'references':list(rows),'auto_train':False,'training_setup':{'ready':False,'reason':'setup_required'}})
   def do_POST(self):
    raw=self.rfile.read(int(self.headers['Content-Length']))
    if self.path.endswith('/delete'):
     body=json.loads(raw);calls.append(('DELETE',self.path,dict(self.headers),body));count=len(rows);rows.clear();self.reply({'deleted':count});return
    data=BytesParser(policy=default).parsebytes(b'Content-Type: '+self.headers['Content-Type'].encode()+b'\r\n\r\n'+raw)
    parts=list(data.iter_parts());fields={part.get_param('name',header='content-disposition'):part.get_payload(decode=True) for part in parts}
    calls.append(('POST',self.path,dict(self.headers),fields))
    image=next(part for part in parts if part.get_param('name',header='content-disposition')=='image')
    row={'id':f'{len(rows)+1:032x}','metadata':{'filename':image.get_filename()}};rows.append(row);self.reply({'asset':row})
  server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
  try:
   with tempfile.TemporaryDirectory(dir=ROOT/'.build') as name:
    folder=Path(name)
    for i in range(100):Image.new('RGB',(12,12),(i,80,120)).save(folder/f'{i}.png')
    process=subprocess.run([str(EXE),'--learning-dataset-test',name,f'http://127.0.0.1:{server.server_port}'],timeout=100,creationflags=subprocess.CREATE_NO_WINDOW)
    report=json.loads((folder/'learning-dataset-result.json').read_text())
    self.assertEqual(process.returncode,0,report);self.assertTrue(report['preview_downloaded']);self.assertTrue(report['limit_100']);self.assertFalse(report['gui_opened'])
    uploads=[row for row in calls if row[0]=='POST'];self.assertEqual(len(uploads),100)
    self.assertTrue(all(row[3]['defer_training']==b'true' for row in uploads))
    self.assertEqual(len({row[3]['image'] for row in uploads}),100)
    self.assertTrue(all(row[2].get('X-Content-Mode')=='hardcore' for row in calls))
    deletions=[row for row in calls if row[0]=='DELETE'];self.assertEqual(len(deletions),1);self.assertEqual(len(deletions[0][3]['asset_ids']),100)
  finally:server.shutdown();server.server_close();thread.join(timeout=2)
