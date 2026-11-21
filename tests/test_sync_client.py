import json,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class SyncClientTests(unittest.TestCase):
 def test_workspace_merge_rules_and_sanitizing(self):
  (ROOT/'.build').mkdir(exist_ok=True)
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--sync-merge-test',folder],timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((Path(folder)/'sync-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
   self.assertTrue(report['ok'],report);self.assertEqual(report['cases'],7)
if __name__=='__main__':unittest.main()
