import json,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ProfileEditorTests(unittest.TestCase):
 def test_edit_cancel_selection_training_and_account_scope(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--profile-editor-test',folder],timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((Path(folder)/'profile-editor-result.json').read_text(encoding='utf-8'))
   self.assertEqual(process.returncode,0,report)
   for key in ('ok','cancel','selection_refresh','training_preserved','account_scope'):self.assertTrue(report[key],key)
