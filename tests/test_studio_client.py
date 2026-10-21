"""Check the real native workspace controls and in-app logout without visible windows."""
from pathlib import Path
import json
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT.parent / 'SNUWCANVAS.exe'


@unittest.skipUnless(EXE.exists(), 'Build native client first')
class StudioTests(unittest.TestCase):
    def test_compact_editor_canvas_and_logout(self):
        with tempfile.TemporaryDirectory(dir=ROOT / '.build') as folder:
            process = subprocess.run([str(EXE), '--studio-test', folder], timeout=30,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            result = json.loads((Path(folder) / 'studio-result.json').read_text())
            self.assertEqual(process.returncode, 0, result)
            self.assertEqual(result['viewports'], 4)
            self.assertTrue(result['logout_keeps_workspace'])
            self.assertTrue(result['large_preview'])
            self.assertTrue(result['account_page'])
            self.assertFalse(result['gui_opened'])
