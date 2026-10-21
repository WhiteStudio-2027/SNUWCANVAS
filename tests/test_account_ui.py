"""Account screen layout and mode switching without showing desktop windows."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / '.build/SNUWCANVAS.exe'


@unittest.skipUnless(EXE.exists(), 'Build native client first')
class AccountUiTests(unittest.TestCase):
    def test_login_signup_layout_at_three_scales(self):
        with tempfile.TemporaryDirectory(dir=ROOT / '.build') as folder:
            process = subprocess.run(
                [str(EXE), '--account-ui-test', folder], timeout=20,
                creationflags=subprocess.CREATE_NO_WINDOW)
            result = json.loads((Path(folder) / 'account-ui-result.json').read_text())
            self.assertEqual(process.returncode, 0, result)
            self.assertTrue(result['tabs_verified'])
            self.assertFalse(result['gui_opened'])
            self.assertEqual(result['scales'], [100, 150, 200])
