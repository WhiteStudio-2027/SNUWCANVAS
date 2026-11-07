"""Real hidden Win32 editor tests plus server/native parser parity."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT.parent/'SNUWCANVAS.exe'
spec=importlib.util.spec_from_file_location('slot_reference',ROOT.parent/'server/src/snuwserver/prompt_document.py')
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)


@unittest.skipUnless(EXE.exists(),'Build native client first')
class PromptEditorTests(unittest.TestCase):
    def test_bidirectional_undo_ime_invalid_draft_restart_and_parser_parity(self):
        cases=[
            {'text':'[char]하루 🌅[/char]\n[scene]12:30 도서관[/scene]'},
            {'text':'legacy [style] and (eyes:1.2)','strict':False},
            {'text':'prefix [char]one[/char] suffix'},
            {'text':'[free]legacy [style] [char]literal[/char][/free][char]new[/char]'},
            {'text':'[CHAR]case insensitive[/CHAR]'},
            {'text':r'\[char]literal\[/char]'},
            {'text':'[char]one'},
            {'text':'[char]one[/scene]'},
            {'text':'[char]one[/char][char]two[/char]'},
            {'text':'[char]one[style]nested[/style][/char]'},
            {'text':'[unknown]one[/unknown]'},
            {'text':'[free]'+'[style]'*300+'[/free]'},
        ]
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            folder=Path(folder);fixture=folder/'cases.json';fixture.write_text(json.dumps(cases,ensure_ascii=False),encoding='utf-8')
            process=subprocess.run([str(EXE),'--prompt-editor-test',str(folder),str(fixture)],timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
            result=json.loads((folder/'prompt-editor-result.json').read_text('utf-8'))
            self.assertEqual(process.returncode,0,result)
            for key in ['bidirectional','undo_redo','ime_deferral','legacy_untouched','invalid_preserved','restart']:
                self.assertTrue(result[key],key)
            for case,actual in zip(cases,result['corpus']):
                with self.subTest(case=case['text'][:80]):
                    try:
                        document=reference.parse(case['text'],strict=case.get('strict',True))
                        expected={'ok':True,'slots':{k:v['text'] for k,v in document['slots'].items()},'unclassified':''.join(p['text'] for p in document['unclassified'])}
                    except ValueError:expected={'ok':False}
                    self.assertEqual(actual,expected)
