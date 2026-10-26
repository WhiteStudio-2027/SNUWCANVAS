param([switch]$Publish,[switch]$CustomServer)
$ErrorActionPreference = 'Stop'
$clientRoot = Split-Path $PSScriptRoot -Parent
$repoRoot = Split-Path $clientRoot -Parent
Set-Location -LiteralPath $clientRoot
$compiler = 'C:\msys64\ucrt64\bin\g++.exe'
$resourceCompiler = 'C:\msys64\ucrt64\bin\windres.exe'
New-Item -ItemType Directory -Force -Path "$clientRoot/.build" | Out-Null
$versionPath = Join-Path $repoRoot 'server/version.json'
$manifest = Get-Content -LiteralPath $versionPath -Raw -Encoding UTF8 | ConvertFrom-Json
$displayVersion = if ($manifest.display_version) { $manifest.display_version } else { $manifest.client_version }
$legalPath = Join-Path $repoRoot ("server/legal/consent-" + $manifest.consent_version + ".md")
$legal = (Get-Content -LiteralPath $legalPath -Raw -Encoding UTF8).Replace("`r`n", "`n")
$publicKeyPath = Join-Path $repoRoot 'server/update-public.der'
$publicHex = if (Test-Path -LiteralPath $publicKeyPath) { ([System.IO.File]::ReadAllBytes($publicKeyPath) | ForEach-Object { $_.ToString('x2') }) -join '' } else { '' }
$modelGuards = Get-Content -LiteralPath (Join-Path $repoRoot 'server/src/snuwserver/model_guards.json') -Raw -Encoding UTF8
$header = '#pragma once' + "`nnamespace snuw {`n" +
    'inline constexpr const char* CLIENT_VERSION = "' + $manifest.client_version + '";' + "`n" +
    'inline constexpr const char* CLIENT_DISPLAY_VERSION = "' + $displayVersion + '";' + "`n" +
    'inline constexpr const char* UPDATE_PUBLIC_KEY_HEX = "' + $publicHex + '";' + "`n" +
    'inline constexpr const char* MODEL_GUARDS_JSON = R"models(' + $modelGuards + ')models";' + "`n" +
    'inline constexpr const char* BUILTIN_CONSENT_VERSION = "' + $manifest.consent_version + '";' + "`n" +
    'inline constexpr const char* BUILTIN_CONSENT_TEXT = R"legal(' + $legal + ')legal";' + "`n}`n"
[System.IO.File]::WriteAllText((Join-Path $clientRoot 'src/native/foundation_generated.hpp'), $header, [System.Text.UTF8Encoding]::new($false))

$numbers = ($manifest.client_version -replace '^v', '') -split '[-+]'
$numeric = $numbers[0] + '.0'
$rcHeader = '#define SNUW_FILEVERSION ' + $numeric.Replace('.', ',') + "`n" + '#define SNUW_VERSION_STRING "' + $manifest.client_version + '"' + "`n" + '#define SNUW_DISPLAY_VERSION "' + $displayVersion + '"' + "`n"
[System.IO.File]::WriteAllText((Join-Path $clientRoot '.build/version.rc.h'), $rcHeader, [System.Text.UTF8Encoding]::new($false))
$windowsManifest = (Get-Content -LiteralPath (Join-Path $clientRoot 'src/native/windows.manifest') -Raw -Encoding UTF8).Replace('@APP_VERSION@', $numeric)
[System.IO.File]::WriteAllText((Join-Path $clientRoot '.build/windows.manifest'), $windowsManifest, [System.Text.UTF8Encoding]::new($false))
& $resourceCompiler -I . src/native/resources.rc -O coff -o .build/resources.o
if ($LASTEXITCODE -ne 0) { throw 'Native resource build failed' }
[string[]]$flavor = @()
if ($CustomServer) { $flavor = @('-DSNUW_CUSTOM_SERVER=1') }
& $compiler @flavor -std=c++20 -O2 -DNDEBUG -municode -mwindows -static -static-libgcc -static-libstdc++ -finput-charset=UTF-8 -I src/native src/native/main.cpp src/native/app.cpp .build/resources.o -o .build/SNUWCANVAS.next.exe -ld2d1 -ld3d11 -ldxgi -ldwrite -lwindowscodecs -lwinhttp -lole32 -loleaut32 -lshell32 -luuid -lcomctl32 -lcomdlg32 -ldwmapi -luxtheme -lgdi32 -luser32 -lshlwapi -lcrypt32 -ladvapi32 -lversion
if ($LASTEXITCODE -ne 0) { throw 'Native C++ build failed' }
Write-Output 'Native EXE compiled.'
$candidate = Join-Path $clientRoot '.build/SNUWCANVAS.next.exe'
$targetName = if ($CustomServer) {'SNUWCANVAS-custom-server.exe'} else {'SNUWCANVAS.exe'}
$target = Join-Path $repoRoot $targetName
if (Test-Path -LiteralPath $target) {
    [System.IO.File]::Replace($candidate, $target, [System.Management.Automation.Language.NullString]::Value)
} else {
    [System.IO.File]::Move($candidate, $target)
}
Write-Output "Updated: $target. User settings preserved."
# -Publish remains accepted for existing commands; no duplicate ZIP/EXE is created.
