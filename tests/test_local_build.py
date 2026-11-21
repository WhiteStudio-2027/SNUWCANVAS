"""Exercise the real update helper on isolated executable copies, never the user app."""
from pathlib import Path
import ctypes
import hashlib
import base64
import json
import os
import shutil
import subprocess
import tempfile
import time
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXE=Path(os.environ.get('LOCAL_BUILD_TEST_EXE') or ROOT.parent/'SNUWCANVAS.exe')
PY_NO_WINDOW=getattr(subprocess,'CREATE_NO_WINDOW',0)
SIGN_SCRIPT=ROOT/'tools'/'sign_local_build.ps1'

class LocalBuildTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory(prefix='local-build-',dir=ROOT/'.build');cls.assets=Path(cls.temp.name)
  rc=cls.assets/'stub.rc';rc.write_text('''#include <windows.h>
1 VERSIONINFO
FILEVERSION 0,3,0,0
PRODUCTVERSION 0,3,0,0
BEGIN
BLOCK "StringFileInfo" BEGIN BLOCK "040904B0" BEGIN VALUE "FileVersion", "v0.3.0" END END
BLOCK "VarFileInfo" BEGIN VALUE "Translation", 0x409, 1200 END
END
''')
  obj=cls.assets/'stub.o'
  subprocess.run(['C:/msys64/ucrt64/bin/windres.exe',str(rc.relative_to(ROOT)),'-O','coff','-o',str(obj.relative_to(ROOT))],cwd=ROOT,check=True,capture_output=True)
  for name,new in [('old',0),('new',1)]:
   binary=cls.assets/(name+'.exe')
   result=subprocess.run(['C:/msys64/ucrt64/bin/g++.exe','-std=c++20','-O1','-static','-municode','-finput-charset=UTF-8',f'-DSTUB_NEW={new}','-I','src/native','tests/local_build_stub.cpp',str(obj.relative_to(ROOT)),'-o',str(binary.relative_to(ROOT)),'-lcrypt32','-lbcrypt','-lshell32','-lole32','-lwinhttp','-luuid','-lversion'],cwd=ROOT,capture_output=True,text=True,timeout=120)
   if result.returncode:raise RuntimeError(result.stderr)
 def sign(self,descriptor):
  """Signed envelope text for a descriptor, made with the machine's local build key."""
  with tempfile.TemporaryDirectory(dir=self.assets) as path:
   payload=Path(path)/'payload.json';out=Path(path)/'envelope.json';payload.write_text(json.dumps(descriptor,separators=(',',':')))
   subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(SIGN_SCRIPT),'-PayloadPath',str(payload),'-OutPath',str(out)],check=True,capture_output=True,timeout=60)
   return out.read_text()
 @classmethod
 def tearDownClass(cls):cls.temp.cleanup()
 def prepare(self,folder):
  local=folder/'.updates/local';local.mkdir(parents=True)
  target=folder/'SNUWCANVAS.exe';shutil.copyfile(self.assets/'old.exe',target)
  new=(self.assets/'new.exe').read_bytes();digest=hashlib.sha256(new).hexdigest()
  previous=hashlib.sha256(target.read_bytes()).hexdigest()
  descriptor={'schema':2,'release_epoch':2,'build_id':'b'*32,'built_at':int(time.time()),'target':target.name,'version':'v0.3.0','sha256':digest,'previous_sha256':previous,'size':len(new)}
  (local/(digest+'.exe')).write_bytes(new)
  envelope=self.sign(descriptor)
  for name in [digest+'.json',target.name+'.ready.json']:(local/name).write_text(envelope)
  helper=local/'helper.exe';shutil.copyfile(EXE,helper)
  return target,local,descriptor,helper
 def run_helper(self,folder,helper,build,parent=0):
  return subprocess.Popen([str(helper),'--apply-local-build',str(folder),str(parent),build['sha256'],build['target']],creationflags=PY_NO_WINDOW)
 def test_waits_for_running_parent_and_preserves_backup(self):
  with tempfile.TemporaryDirectory(dir=self.assets) as path:
   folder=Path(path);target,local,build,helper=self.prepare(folder);gate=folder/'exit-gate'
   parent=subprocess.Popen([str(target),'--hold',str(gate)],creationflags=PY_NO_WINDOW)
   worker=self.run_helper(folder,helper,build,parent.pid)
   try:
    time.sleep(.3);self.assertIsNone(worker.poll());self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),build['previous_sha256'])
    duplicate=self.run_helper(folder,helper,build,parent.pid);self.assertEqual(duplicate.wait(5),2)
    gate.write_text('exit');parent.wait(5);self.assertEqual(worker.wait(15),0)
    self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),build['sha256'])
    self.assertEqual(hashlib.sha256((local/'backups'/(build['previous_sha256']+'.exe')).read_bytes()).hexdigest(),build['previous_sha256'])
    status=json.loads((local/(target.name+'.status.json')).read_text());self.assertEqual(status['state'],'healthy');self.assertEqual(status['build_id'],build['build_id'])
   finally:
    gate.write_text('exit');parent.wait(5)
    if worker.poll() is None:worker.wait(15)
 def test_failed_boot_restores_old_executable(self):
  with tempfile.TemporaryDirectory(dir=self.assets) as path:
   folder=Path(path);target,local,build,helper=self.prepare(folder);(folder/'fail-new').write_text('fail')
   worker=self.run_helper(folder,helper,build);self.assertEqual(worker.wait(15),1)
   self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),build['previous_sha256'])
   deadline=time.monotonic()+3
   while not (folder/'booted.json').exists() and time.monotonic()<deadline:time.sleep(.02)
   self.assertFalse(json.loads((folder/'booted.json').read_text())['new'])
 def test_tampered_build_keeps_current_executable(self):
  with tempfile.TemporaryDirectory(dir=self.assets) as path:
   folder=Path(path);target,local,build,helper=self.prepare(folder);(local/(build['sha256']+'.exe')).write_bytes(b'MZ tampered')
   worker=self.run_helper(folder,helper,build);self.assertEqual(worker.wait(15),1)
   self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),build['previous_sha256'])
 def test_unsigned_or_forged_descriptor_keeps_current_executable(self):
  def attempt(name,mutate):
   with tempfile.TemporaryDirectory(dir=self.assets) as path:
    folder=Path(path);target,local,build,helper=self.prepare(folder);mutate(local,build)
    worker=self.run_helper(folder,helper,build);self.assertEqual(worker.wait(15),1,name)
    self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),build['previous_sha256'],name)
  def plain(local,build):(local/(build['sha256']+'.json')).write_text(json.dumps(build))
  def edited(local,build):
   envelope=json.loads((local/(build['sha256']+'.json')).read_text());data=json.loads(base64.b64decode(envelope['payload']));data['version']='v9.9.9'
   envelope['payload']=base64.b64encode(json.dumps(data,separators=(',',':')).encode()).decode();(local/(build['sha256']+'.json')).write_text(json.dumps(envelope))
  def old_schema(local,build):(local/(build['sha256']+'.json')).write_text(self.sign({**build,'schema':1}))
  def no_signature(local,build):
   envelope=json.loads((local/(build['sha256']+'.json')).read_text());envelope['signature']='';(local/(build['sha256']+'.json')).write_text(json.dumps(envelope))
  for name,mutate in [('plain json',plain),('edited payload',edited),('old schema',old_schema),('empty signature',no_signature)]:attempt(name,mutate)
 def test_real_app_watcher_defers_busy_saves_draft_and_restarts(self):
  with tempfile.TemporaryDirectory(dir=self.assets) as path:
   folder=Path(path);target,local,build,helper=self.prepare(folder)
   process=subprocess.run([str(EXE),'--local-build-watch-test',str(folder)],timeout=20,creationflags=PY_NO_WINDOW)
   result=json.loads((folder/'watch-result.json').read_text());self.assertEqual(process.returncode,0,result)
   self.assertTrue(result['busy_deferred']);self.assertTrue(result['draft_saved'])
   status=local/(target.name+'.status.json');deadline=time.monotonic()+10
   while time.monotonic()<deadline:
    if status.exists() and json.loads(status.read_text()).get('state')=='healthy':break
    time.sleep(.05)
   self.assertEqual(json.loads(status.read_text())['state'],'healthy')
   self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),build['sha256'])
   kernel=ctypes.windll.kernel32;kernel.OpenProcess.restype=ctypes.c_void_p
   handle=kernel.OpenProcess(0x100000,False,json.loads(status.read_text())['helper_pid'])
   if handle:
    self.assertEqual(kernel.WaitForSingleObject(ctypes.c_void_p(handle),5000),0);kernel.CloseHandle(ctypes.c_void_p(handle))
