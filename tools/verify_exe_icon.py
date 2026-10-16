import ctypes,struct
from pathlib import Path
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.LoadLibraryExW.argtypes=[ctypes.c_wchar_p,ctypes.c_void_p,ctypes.c_uint];k.LoadLibraryExW.restype=ctypes.c_void_p
k.FindResourceW.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p];k.FindResourceW.restype=ctypes.c_void_p
k.SizeofResource.argtypes=[ctypes.c_void_p,ctypes.c_void_p];k.SizeofResource.restype=ctypes.c_uint
k.LoadResource.argtypes=[ctypes.c_void_p,ctypes.c_void_p];k.LoadResource.restype=ctypes.c_void_p
k.LockResource.argtypes=[ctypes.c_void_p];k.LockResource.restype=ctypes.c_void_p
k.FreeLibrary.argtypes=[ctypes.c_void_p]
def verify():
 h=k.LoadLibraryExW(str((Path(__file__).resolve().parents[1]/'.build/SNUWCANVAS.exe')),None,2)
 assert h,'Executable missing'
 def res(n,t):
  r=k.FindResourceW(h,n,t);assert r,'Icon resource missing'
  return ctypes.string_at(k.LockResource(k.LoadResource(h,r)),k.SizeofResource(h,r))
 try:
  group=res(1,14);count=struct.unpack_from('<H',group,4)[0];ico=(Path(__file__).resolve().parents[1]/'assets/app.ico').read_bytes()
  sizes=[]
  for i in range(count):
   w,ht,c,z,planes,bits,size,rid=struct.unpack_from('<BBBBHHIH',group,6+i*14)
   assert res(rid,3) in ico,'Custom icon resource mismatch'
   sizes.append(w or 256)
  assert {16,24,32,48,64,128,256}==set(sizes)
  print('EXE embedded custom icon verified:',sizes)
 finally:k.FreeLibrary(h)
if __name__=='__main__':verify()
