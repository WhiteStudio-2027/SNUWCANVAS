from pathlib import Path
import os,unittest
EXE=Path(__file__).resolve().parents[2]/'SNUWCANVAS.exe'
@unittest.skipUnless(os.name=='nt' and EXE.is_file(),'Build Windows EXE first')
class BrandResourceTests(unittest.TestCase):
    def test_white_studio_company_and_copyright_resources(self):
        import ctypes
        from ctypes import wintypes
        version=ctypes.WinDLL('version',use_last_error=True)
        version.GetFileVersionInfoSizeW.argtypes=[wintypes.LPCWSTR,ctypes.POINTER(wintypes.DWORD)];version.GetFileVersionInfoSizeW.restype=wintypes.DWORD
        version.GetFileVersionInfoW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p];version.GetFileVersionInfoW.restype=wintypes.BOOL
        version.VerQueryValueW.argtypes=[ctypes.c_void_p,wintypes.LPCWSTR,ctypes.POINTER(ctypes.c_void_p),ctypes.POINTER(wintypes.UINT)];version.VerQueryValueW.restype=wintypes.BOOL
        ignored=wintypes.DWORD();size=version.GetFileVersionInfoSizeW(str(EXE),ctypes.byref(ignored));self.assertGreater(size,0)
        data=ctypes.create_string_buffer(size);self.assertTrue(version.GetFileVersionInfoW(str(EXE),0,size,data))
        for name in ('CompanyName','LegalCopyright'):
            pointer=ctypes.c_void_p();length=wintypes.UINT();self.assertTrue(version.VerQueryValueW(data,'\\StringFileInfo\\040904B0\\'+name,ctypes.byref(pointer),ctypes.byref(length)))
            self.assertIn('White Studio',ctypes.wstring_at(pointer,length.value).rstrip('\0'))
