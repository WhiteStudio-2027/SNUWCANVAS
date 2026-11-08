import base64,json,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class StoryboardTests(unittest.TestCase):
 def test_pages_panels_inheritance_isolation_backup_gallery_and_restart(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   folder=Path(folder);(folder/'fixture.png').write_bytes(base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+a1ioAAAAASUVORK5CYII='))
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--storyboard-test',str(folder)],timeout=45,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   result=json.loads((folder/'storyboard-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,result)
   for key in ('ok','inheritance','isolation','delete_undo','backup','restart','legacy_edit','gallery','entity_snapshot'):self.assertTrue(result[key],key)
