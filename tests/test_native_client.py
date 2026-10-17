"""Native client verification without desktop windows, including offscreen GPU drawing."""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT/'.build/SNUWCANVAS.exe'

@unittest.skipUnless(EXE.exists(),'Build native client first')
class NativeTests(unittest.TestCase):
    def execute(self,mode,folder,result):
        process=subprocess.run([str(EXE),mode,str(folder)],timeout=40,creationflags=subprocess.CREATE_NO_WINDOW)
        report=json.loads((folder/result).read_text(encoding='utf-8'))
        self.assertEqual(process.returncode,0,report)
        self.assertTrue(report['ok'],report)
        self.assertFalse(report.get('gui_opened',False))
        return report
    def test_native_settings_controls_and_restart(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            report=self.execute('--self-test',Path(folder),'native-test-result.json')
            self.assertEqual(report['tests'],9)
            self.assertFalse(report['python_runtime'])
    def test_offscreen_hardware_rendering_and_bundled_font(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            report=self.execute('--graphics-test',Path(folder),'native-graphics-result.json')
            self.assertTrue(report['hardware_gpu'])
            self.assertTrue(report['bundled_korean_font'])
    def test_image_queue_and_invalid_files_recovery(self):
        from PIL import Image
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            path=Path(folder)
            Image.new('RGB',(32,48),'#336699').save(path/'valid.png')
            (path/'invalid.png').write_text('not an image',encoding='utf-8')
            report=self.execute('--image-test',path,'native-image-result.json')
            self.assertEqual(report['images'],100)
            self.assertTrue(report['failure_reported'])
            self.assertTrue(report['preview_recovered'])
    def test_project_folders_order_search_and_restart(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            report=self.execute('--sidebar-test',Path(folder),'native-sidebar-result.json')
            self.assertTrue(report['folder_restart'])
            self.assertTrue(report['project_state_preserved'])
    def test_thumbnail_cache_older_pages_after_capacity(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            report=self.execute('--cache-test',Path(folder),'native-cache-result.json')
            self.assertEqual(report['pages'],10)
            self.assertEqual(report['capacity'],256)
    def test_actual_gallery_decode_cache_and_gpu_pixels(self):
        from PIL import Image
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            path=Path(folder)
            Image.new('RGB',(160,200),'#336699').save(path/'image-0.png')
            for index in range(1,360):
                shutil.copyfile(path/'image-0.png',path/f'image-{index}.png')
            report=self.execute('--library-render-test',path,'native-library-render-result.json')
            self.assertTrue(report['actual_app_paint'])
            self.assertEqual(report['gpu_pixel_pages'],135)
            self.assertEqual(report['learning_pages'],9)
            self.assertEqual(report['library_widths'],3)
            self.assertEqual(report['dpi_scales'],3)
    def test_responsive_grid_hit_testing_and_dark_options(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
            report=self.execute('--layout-test',Path(folder),'native-layout-result.json')
            self.assertEqual(report['responsive_widths'],5)
            self.assertEqual(report['thumbnail_capacity'],256)
            self.assertTrue(report['dark_options'])
    def test_existing_projects_on_temporary_copies(self):
        for source in (ROOT/'config',ROOT/'data/config'):
            if not (source/'channels.json').exists():continue
            checksum=hashlib.sha256((source/'channels.json').read_bytes()).digest()
            with tempfile.TemporaryDirectory(dir=ROOT/'.build') as folder:
                path=Path(folder);(path/'config').mkdir()
                for name in ('channels.json','canvas_settings.json','credentials.json'):
                    if (source/name).exists():shutil.copy2(source/name,path/'config'/name)
                self.execute('--migration-test',path,'native-migration-result.json')
            self.assertEqual(hashlib.sha256((source/'channels.json').read_bytes()).digest(),checksum)
    def test_native_api_multipart_and_batch_and_edit(self):
        spec=importlib.util.spec_from_file_location('native_api_test',ROOT/'tools/test_native_api.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        module.main()

if __name__=='__main__':unittest.main()
