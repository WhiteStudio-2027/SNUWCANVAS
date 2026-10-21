from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import subprocess
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT.parent / 'SNUWCANVAS.exe'


@unittest.skipUnless(EXE.exists(), 'Build native client first')
class AccountSessionClientTests(unittest.TestCase):
    def test_developer_table_and_non_developer_gate_without_visible_window(self):
        with tempfile.TemporaryDirectory(dir=ROOT / '.build') as name:
            result=subprocess.run([str(EXE),'--developer-test',name],timeout=25,creationflags=subprocess.CREATE_NO_WINDOW)
            report=json.loads((Path(name)/'developer-result.json').read_text(encoding='utf-8'))
            self.assertEqual(result.returncode,0,report)
            self.assertFalse(report['gui_opened'])
            self.assertTrue(report['ordinary_user_blocked'])
            self.assertTrue(report['community_management'])

    def test_rotated_tokens_are_protected_origin_bound_and_serialized(self):
        calls = []

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def reply(self, value):
                raw = json.dumps(value).encode()
                self.send_response(200)
                self.send_header('Content-Length', str(len(raw)))
                self.end_headers()
                self.wfile.write(raw)

            def do_POST(self):
                body = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
                calls.append(('POST', self.path, dict(self.headers), body))
                if self.path=='/api/auth/logout-all':
                    self.reply({'logged_out':True});return
                self.reply({'access_token': 'b' * 32, 'refresh_token': 's' * 60,
                            'expires_in': 900, 'account_id': 'test-account'})

            def do_GET(self):
                calls.append(('GET', self.path, dict(self.headers), None))
                self.reply({'ok': self.headers.get('Authorization') == 'Bearer ' + 'b' * 32})

        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            with tempfile.TemporaryDirectory(dir=ROOT / '.build') as name:
                folder = Path(name)
                result = subprocess.run([str(EXE), '--account-session-test', name,
                                         f'http://127.0.0.1:{server.server_port}'],
                                        timeout=25, creationflags=subprocess.CREATE_NO_WINDOW)
                report = json.loads((folder / 'account-session-result.json').read_text(encoding='utf-8'))
                self.assertEqual(result.returncode, 0, report)
                self.assertFalse(report['gui_opened'])
                self.assertTrue(report['cross_origin_blocked'])
                self.assertTrue(report['logout_all'])
                logouts=[row for row in calls if row[1]=='/api/auth/logout-all']
                self.assertEqual(len(logouts),1)
                self.assertEqual(logouts[0][2].get('Authorization'),'Bearer '+'b'*32)
                self.assertEqual(logouts[0][3],{})
                refreshes = [row for row in calls if row[1] == '/api/auth/refresh']
                self.assertEqual(len(refreshes), 1)
                self.assertEqual(refreshes[0][1], '/api/auth/refresh')
                self.assertEqual(refreshes[0][3], {'refresh_token': 'r' * 60})
                requests = [row for row in calls if row[0] == 'GET']
                self.assertEqual(len(requests), 2)
                self.assertTrue(all('X-API-Key' not in row[2] for row in calls))
                self.assertTrue(all(row[2].get('X-Client-Id') for row in calls))
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=2)
