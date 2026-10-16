"""Compatibility build entry; the client itself is entirely native C++."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
subprocess.run(['powershell','-NoProfile','-ExecutionPolicy','Bypass','-File',str(root/'tools/build_native.ps1'),'-Publish'],cwd=root,check=True)
