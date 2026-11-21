param([switch]$Publish,[switch]$CustomServer)
$ErrorActionPreference = 'Stop'
$clientRoot = Split-Path $PSScriptRoot -Parent
$repoRoot = Split-Path $clientRoot -Parent
$serverWorkspace = $repoRoot
if (-not (Test-Path -LiteralPath (Join-Path $serverWorkspace "server/version.json"))) { $serverWorkspace = Split-Path $repoRoot -Parent }
Set-Location -LiteralPath $clientRoot
$compiler = 'C:\msys64\ucrt64\bin\g++.exe'
$resourceCompiler = 'C:\msys64\ucrt64\bin\windres.exe'
New-Item -ItemType Directory -Force -Path "$clientRoot/.build" | Out-Null
$versionPath = Join-Path $serverWorkspace 'server/version.json'
$manifest = Get-Content -LiteralPath $versionPath -Raw -Encoding UTF8 | ConvertFrom-Json
$displayVersion = if ($manifest.display_version) { $manifest.display_version } else { $manifest.client_version }
$legalPath = Join-Path $serverWorkspace ("server/legal/consent-" + $manifest.consent_version + ".md")
$legal = (Get-Content -LiteralPath $legalPath -Raw -Encoding UTF8).Replace("`r`n", "`n")
$publicKeyPath = Join-Path $serverWorkspace 'server/update-public.der'
$publicHex = if (Test-Path -LiteralPath $publicKeyPath) { ([System.IO.File]::ReadAllBytes($publicKeyPath) | ForEach-Object { $_.ToString('x2') }) -join '' } else { '' }
$localKeyPath = Join-Path $clientRoot 'local-build-public.der'
$localPublicHex = if (Test-Path -LiteralPath $localKeyPath) { ([System.IO.File]::ReadAllBytes($localKeyPath) | ForEach-Object { $_.ToString('x2') }) -join '' } else { '' }
. (Join-Path $PSScriptRoot 'local_build_signing.ps1')
$modelGuards = Get-Content -LiteralPath (Join-Path $serverWorkspace 'server/src/snuwserver/model_guards.json') -Raw -Encoding UTF8
$header = '#pragma once' + "`nnamespace snuw {`n" +
    'inline constexpr const char* CLIENT_VERSION = "' + $manifest.client_version + '";' + "`n" +
    'inline constexpr const char* CLIENT_DISPLAY_VERSION = "' + $displayVersion + '";' + "`n" +
    'inline constexpr const char* UPDATE_PUBLIC_KEY_HEX = "' + $publicHex + '";' + "`n" +
    'inline constexpr const char* LOCAL_BUILD_PUBLIC_KEY_HEX = "' + $localPublicHex + '";' + "`n" +
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
& $compiler @flavor -std=c++20 -O2 -DNDEBUG -municode -mwindows -static -static-libgcc -static-libstdc++ -finput-charset=UTF-8 -I src/native src/native/main.cpp src/native/app.cpp .build/resources.o -o .build/SNUWCANVAS.next.exe -ld2d1 -ld3d11 -ldxgi -ldwrite -lwindowscodecs -lwinhttp -lole32 -loleaut32 -lshell32 -luuid -lcomctl32 -lcomdlg32 -ldwmapi -luxtheme -lgdi32 -luser32 -lshlwapi -lcrypt32 -ladvapi32 -lbcrypt -lversion
if ($LASTEXITCODE -ne 0) { throw 'Native C++ build failed' }
Write-Output 'Native EXE compiled.'
$candidate = Join-Path $clientRoot '.build/SNUWCANVAS.next.exe'
$targetName = if ($CustomServer) {'SNUWCANVAS-custom-server.exe'} else {'SNUWCANVAS.exe'}
$target = Join-Path $repoRoot $targetName
$localRoot = Join-Path $repoRoot '.updates/local'
New-Item -ItemType Directory -Force -Path (Join-Path $localRoot 'backups') | Out-Null
$updateFolder = Get-Item -LiteralPath (Split-Path $localRoot -Parent) -Force
$updateFolder.Attributes = $updateFolder.Attributes -bor [System.IO.FileAttributes]::Hidden
$stageLock = $null
try {
    # One staging at a time per target: a second build waits for the first instead of overwriting its ready file.
    try { $stageLock = [System.IO.File]::Open((Join-Path $localRoot ($targetName + '.stage.lock')), 'OpenOrCreate', 'ReadWrite', 'None') }
    catch [System.IO.IOException] { throw 'Another local build is staging the same target. Try again in a moment.' }
    $newHash = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash.ToLowerInvariant()
    $oldHash = if (Test-Path -LiteralPath $target) { (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() } else { '' }
    $staged = Join-Path $localRoot ($newHash + '.exe')
    [System.IO.File]::Copy($candidate, $staged, $true)
    $buildId = [guid]::NewGuid().ToString('N')
    $descriptor = [ordered]@{schema=2; release_epoch=2; build_id=$buildId; built_at=[DateTimeOffset]::UtcNow.ToUnixTimeSeconds(); target=$targetName; version=$manifest.client_version; sha256=$newHash; previous_sha256=$oldHash; size=(Get-Item -LiteralPath $staged).Length}
    # Running apps only trust descriptors signed with the local build key; without the key the EXE can still be swapped on disk.
    $signer = Get-LocalBuildSigner $localKeyPath
    $json = $null
    if ($signer) {
        $json = New-LocalBuildEnvelope ([System.Text.Encoding]::UTF8.GetBytes(($descriptor | ConvertTo-Json -Compress))) $signer
        [System.IO.File]::WriteAllText((Join-Path $localRoot ($newHash + '.json')), $json, [System.Text.UTF8Encoding]::new($false))
    } else { Write-Warning 'No local build signing key (run tools/new_local_build_key.ps1): running apps will not be told to restart.' }
    $applied = $false
    $backup = if ($oldHash) { Join-Path $localRoot ('backups/' + $oldHash + '.exe') } else { '' }
    try {
        if ($oldHash -and $oldHash -ne $newHash) {
            if (Test-Path -LiteralPath $backup) {
                if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash.ToLowerInvariant() -ne $oldHash) { throw 'Previous build backup integrity failed' }
                [System.IO.File]::Replace($candidate, $target, [System.Management.Automation.Language.NullString]::Value)
            } else { [System.IO.File]::Replace($candidate, $target, $backup) }
        } elseif (-not $oldHash) { [System.IO.File]::Move($candidate, $target) }
        $applied = $true
    } catch {
        $failure = $_.Exception
        $running = $false
        try { $running = [bool](Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $target }) } catch { $running = $false }
        $inUse = ($failure -is [System.IO.IOException] -or $failure -is [System.UnauthorizedAccessException]) -and ($running -or (($failure.HResult -band 0xFFFF) -in 32, 33))
        if (-not $inUse) {
            # Not a lock: never queue a restart for it. Make sure the current EXE is intact, then fail the build.
            $intact = (Test-Path -LiteralPath $target) -and ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -eq $oldHash)
            if ($oldHash -and -not $intact) {
                if ($backup -and (Test-Path -LiteralPath $backup) -and ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash.ToLowerInvariant() -eq $oldHash)) {
                    [System.IO.File]::Copy($backup, $target, $true)
                    Write-Warning 'Apply failed midway; the previous EXE was restored from its backup.'
                } else { Write-Warning "Apply failed midway and no verified backup exists. Check $target." }
            }
            throw
        }
        Write-Output 'The running EXE is locked. The validated build is queued for automatic restart.'
    }
    $ready = Join-Path $localRoot ($targetName + '.ready.json')
    # Always signal the running app: even when the EXE was swapped on disk, a running copy restarts only on this file.
    if ($json) {
        $readyNext = $ready + '.' + $buildId + '.next'
        [System.IO.File]::WriteAllText($readyNext, $json, [System.Text.UTF8Encoding]::new($false))
        if (Test-Path -LiteralPath $ready) { [System.IO.File]::Replace($readyNext, $ready, [System.Management.Automation.Language.NullString]::Value) } else { [System.IO.File]::Move($readyNext, $ready) }
    }
    # Keep only the three newest staged builds and helper copies.
    foreach ($folder in @($localRoot, (Join-Path $localRoot 'helpers'))) {
        if (-not (Test-Path -LiteralPath $folder)) { continue }
        Get-ChildItem -LiteralPath $folder -Filter '*.exe' -File | Where-Object { $_.BaseName -match '^[a-f0-9]{64}$' } | Sort-Object LastWriteTime -Descending | Select-Object -Skip 3 | ForEach-Object {
            try { Remove-Item -LiteralPath $_.FullName -Force; $stale = [System.IO.Path]::ChangeExtension($_.FullName, '.json'); if (Test-Path -LiteralPath $stale) { Remove-Item -LiteralPath $stale -Force } } catch { }
        }
    }
    Write-Output "Build ready: $target. Applied to disk: $applied. Settings preserved; compatible running clients restart when idle."
} finally {
    if ($stageLock) { $stageLock.Dispose() }
}
# -Publish remains accepted for existing commands; no duplicate ZIP/EXE is created.
