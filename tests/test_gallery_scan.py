import json,subprocess,tempfile,unittest
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
class GalleryScanTests(unittest.TestCase):
 def test_background_thousand_files_stale_modes_accounts_and_search(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   folder=Path(folder);Image.new('RGB',(16,16),'#3399bb').save(folder/'fixture.png')
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--gallery-scan-test',str(folder)],cwd=folder,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));report=json.loads((folder/'gallery-scan-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
   print(json.dumps(report,sort_keys=True));self.assertEqual(report['fixtures'],1001);self.assertGreater(report['message_beats'],0)
   for key in ('ok','account_mode_isolation','search','cache_invalidation','model_page_restore','learning_preview_preserved','selection_saved_during_scan','logout_selection_preserved'):self.assertTrue(report[key],key)
