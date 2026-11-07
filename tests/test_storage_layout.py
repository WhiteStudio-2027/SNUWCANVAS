import json,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class StorageLayoutTests(unittest.TestCase):
 def test_stable_ids_modes_legacy_mapping_and_delete(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   result=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--storage-layout-test',folder],timeout=20,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((Path(folder)/'storage-layout-result.json').read_text(encoding='utf-8'))
   self.assertEqual(result.returncode,0,report)
   for key in ('ok','rename_stable','mode_isolation','legacy','metadata_delete','migration_mapping'):self.assertTrue(report[key],key)
