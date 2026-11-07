import base64,hashlib,json,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'server/src'))
from snuwserver import layout_migration
class GalleryMigrationTests(unittest.TestCase):
 def test_copy_tool_to_native_gallery_and_disconnect(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
   folder=Path(folder).resolve();source=folder/'original';source.mkdir()
   png=base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+a1ioAAAAASUVORK5CYII=')
   for name in ('one','nested/two'):
    file=source/(name+'.png');file.parent.mkdir(parents=True,exist_ok=True);file.write_bytes(png);file.with_suffix('.json').write_text('{"original":true}',encoding='utf-8')
   before={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
   scope='a'*64;channel='c'*32;fixture={'scope':scope,'channel':channel}
   for mode in ('safe','hardcore'):
    plan=layout_migration.plan(source,folder/'copies',scope,channel,mode)
    fixture[mode]=layout_migration.apply(source,folder/'copies',scope,channel,mode,plan['digest'])['target']
   (folder/'fixture.json').write_text(json.dumps(fixture),encoding='utf-8')
   process=subprocess.run([str(ROOT.parent/'SNUWCANVAS.exe'),'--gallery-migration-test',str(folder)],timeout=45,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
   report=json.loads((folder/'gallery-migration-result.json').read_text(encoding='utf-8'));self.assertEqual(process.returncode,0,report)
   for key in ('ok','ownership','mode_isolation','tamper_rejected','restart','disconnect_preserves_files'):self.assertTrue(report[key],key)
   after={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()};self.assertEqual(before,after)
