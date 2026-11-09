import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FinalNativeTests(unittest.TestCase):
    def test_migration_copy_rollback_and_seed_ranking(self):
        compiler = Path('C:/msys64/ucrt64/bin/g++.exe')
        if not compiler.exists():
            self.skipTest('Windows native compiler required')
        with tempfile.TemporaryDirectory(prefix='snuw-final-') as folder:
            executable = Path(folder) / 'features.exe'
            compiled = subprocess.run([str(compiler), '-std=c++20', '-O0', '-static', '-finput-charset=UTF-8',
                            '-I', str(ROOT / 'src/native'), str(ROOT / 'tests/final_features_native.cpp'),
                            '-o', str(executable), '-lole32', '-lshell32', '-ladvapi32', '-lcrypt32', '-lwinhttp', '-luuid'],
                           capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=180)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            result = subprocess.run([str(executable), str(Path(folder) / 'fixture')],
                                    capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(json.loads(result.stdout)['ok'])


if __name__ == '__main__':
    unittest.main()
