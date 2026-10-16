param([switch]$Publish, [string]$InstallDir)
$ErrorActionPreference = 'Stop'
$clientRoot = Split-Path $PSScriptRoot -Parent
$repoRoot = Split-Path $clientRoot -Parent
Set-Location -LiteralPath $clientRoot
$compiler = 'C:\msys64\ucrt64\bin\g++.exe'
$resourceCompiler = 'C:\msys64\ucrt64\bin\windres.exe'
New-Item -ItemType Directory -Force -Path "$clientRoot/.build" | Out-Null
& $resourceCompiler -I . src/native/resources.rc -O coff -o .build/resources.o
if ($LASTEXITCODE -ne 0) { throw 'Native resource build failed' }
& $compiler -std=c++20 -O2 -DNDEBUG -municode -mwindows -static -static-libgcc -static-libstdc++ -finput-charset=UTF-8 -I src/native src/native/main.cpp src/native/app.cpp .build/resources.o -o .build/SNUWCANVAS.exe -ld2d1 -ld3d11 -ldxgi -ldwrite -lwindowscodecs -lwinhttp -lole32 -loleaut32 -lshell32 -luuid -lcomctl32 -lcomdlg32 -ldwmapi -luxtheme -lgdi32 -luser32 -lshlwapi
if ($LASTEXITCODE -ne 0) { throw 'Native C++ build failed' }
Write-Output 'Native EXE compiled.'
if ($Publish) {
    New-Item -ItemType Directory -Force -Path "$clientRoot/dist" | Out-Null
    Copy-Item -LiteralPath "$clientRoot/.build/SNUWCANVAS.exe" -Destination "$clientRoot/dist/SNUWCANVAS.exe" -Force
    Compress-Archive -LiteralPath "$clientRoot/dist/SNUWCANVAS.exe" -DestinationPath "$clientRoot/dist/SNUWCANVAS_client.zip" -CompressionLevel Optimal -Force
    if ($InstallDir) {
        $resolvedInstall = (Resolve-Path -LiteralPath $InstallDir).Path
        Copy-Item -LiteralPath "$clientRoot/.build/SNUWCANVAS.exe" -Destination (Join-Path $resolvedInstall 'SNUWCANVAS.exe') -Force
    }
    Write-Output 'Published to client/dist; existing artifacts overwritten. User data preserved.'
}
