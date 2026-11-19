from pathlib import Path
import subprocess
import tempfile
import shutil
import unittest
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]

class LongPathTemporaryDirectory(tempfile.TemporaryDirectory):
    @classmethod
    def _rmtree(cls, name, ignore_errors=False, repeated=False):
        folder = Path(name).resolve()
        if folder.parent != ROOT / '.build' or not folder.name.startswith('gallery-scope-'):
            raise ValueError('Unexpected test cleanup target')
        shutil.rmtree('\\\\?\\' + str(folder), ignore_errors=ignore_errors)

class GalleryScopeTests(unittest.TestCase):
    def test_current_and_all_projects_preserve_mode_and_account_filters(self):
        compiler = Path('C:/msys64/ucrt64/bin/g++.exe')
        if not compiler.exists():
            self.skipTest('Windows native compiler required')
        with LongPathTemporaryDirectory(prefix='gallery-scope-', dir=ROOT / '.build') as name:
            folder = Path(name)
            Image.new('RGB', (8, 8), '#3399bb').save(folder / 'fixture.png')
            binary = folder / 'scope.exe'
            build = subprocess.run([str(compiler), '-std=c++20', '-O2', '-static', '-municode', '-finput-charset=UTF-8',
                '-I', 'src/native', 'tests/gallery_scope_native.cpp', '-o', str(binary.relative_to(ROOT)),
                '-lwindowscodecs', '-lole32', '-luuid', '-lwinhttp', '-lcrypt32', '-lbcrypt', '-lshell32'],
                cwd=ROOT, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=120)
            self.assertEqual(build.returncode, 0, build.stderr)
            result = subprocess.run([str(binary), str(folder)], capture_output=True,
                text=True, errors='replace', timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
