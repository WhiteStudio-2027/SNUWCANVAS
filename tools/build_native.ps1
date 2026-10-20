param([switch]$Publish, [string]$InstallDir)
$ErrorActionPreference = 'Stop'
$clientRoot = Split-Path $PSScriptRoot -Parent
$repoRoot = Split-Path $clientRoot -Parent
Set-Location -LiteralPath $clientRoot
$compiler = 'C:\msys64\ucrt64\bin\g++.exe'
$resourceCompiler = 'C:\msys64\ucrt64\bin\windres.exe'
New-Item -ItemType Directory -Force -Path "$clientRoot/.build" | Out-Null
$versionPath = Join-Path $repoRoot 'server/version.json'
$manifest = Get-Content -LiteralPath $versionPath -Raw -Encoding UTF8 | ConvertFrom-Json
$legalPath = Join-Path $repoRoot ("server/legal/consent-" + $manifest.consent_version + ".md")
$legal = (Get-Content -LiteralPath $legalPath -Raw -Encoding UTF8).Replace("`r`n", "`n")
$publicKeyPath = Join-Path $repoRoot 'server/update-public.der'
$publicHex = if (Test-Path -LiteralPath $publicKeyPath) { ([System.IO.File]::ReadAllBytes($publicKeyPath) | ForEach-Object { $_.ToString('x2') }) -join '' } else { '' }
$header = '#pragma once' + "`nnamespace snuw {`n" +
    'inline constexpr const char* CLIENT_VERSION = "' + $manifest.client_version + '";' + "`n" +
    'inline constexpr const char* UPDATE_PUBLIC_KEY_HEX = "' + $publicHex + '";' + "`n" +
    'inline constexpr const char* BUILTIN_CONSENT_VERSION = "' + $manifest.consent_version + '";' + "`n" +
    'inline constexpr const char* BUILTIN_CONSENT_TEXT = R"legal(' + $legal + ')legal";' + "`n}`n"
[System.IO.File]::WriteAllText((Join-Path $clientRoot 'src/native/foundation_generated.hpp'), $header, [System.Text.UTF8Encoding]::new($false))

$numbers = ($manifest.client_version -replace '^v', '') -split '[-+]'
$numeric = $numbers[0] + '.0'
$rcHeader = '#define SNUW_FILEVERSION ' + $numeric.Replace('.', ',') + "`n" + '#define SNUW_VERSION_STRING "' + $manifest.client_version + '"' + "`n"
[System.IO.File]::WriteAllText((Join-Path $clientRoot '.build/version.rc.h'), $rcHeader, [System.Text.UTF8Encoding]::new($false))
$windowsManifest = (Get-Content -LiteralPath (Join-Path $clientRoot 'src/native/windows.manifest') -Raw -Encoding UTF8).Replace('@APP_VERSION@', $numeric)
[System.IO.File]::WriteAllText((Join-Path $clientRoot '.build/windows.manifest'), $windowsManifest, [System.Text.UTF8Encoding]::new($false))
& $resourceCompiler -I . src/native/resources.rc -O coff -o .build/resources.o
if ($LASTEXITCODE -ne 0) { throw 'Native resource build failed' }
& $compiler -std=c++20 -O2 -DNDEBUG -municode -mwindows -static -static-libgcc -static-libstdc++ -finput-charset=UTF-8 -I src/native src/native/main.cpp src/native/app.cpp .build/resources.o -o .build/SNUWCANVAS.exe -ld2d1 -ld3d11 -ldxgi -ldwrite -lwindowscodecs -lwinhttp -lole32 -loleaut32 -lshell32 -luuid -lcomctl32 -lcomdlg32 -ldwmapi -luxtheme -lgdi32 -luser32 -lshlwapi -lcrypt32 -ladvapi32 -lversion
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
