# Signs a payload file with the local build key and writes the envelope (used by tests; build_native.ps1 signs inline).
param([Parameter(Mandatory)][string]$PayloadPath, [Parameter(Mandatory)][string]$OutPath)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'local_build_signing.ps1')
$signer = Get-LocalBuildSigner (Join-Path (Split-Path $PSScriptRoot -Parent) 'local-build-public.der')
if (-not $signer) { throw 'No local build signing key is installed on this machine.' }
$envelope = New-LocalBuildEnvelope ([System.IO.File]::ReadAllBytes($PayloadPath)) $signer
[System.IO.File]::WriteAllText($OutPath, $envelope, [System.Text.UTF8Encoding]::new($false))
