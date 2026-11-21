# One-time setup: creates the local build signing key (RSA 3072, non-exportable) in the CurrentUser certificate
# store and writes its public key to client\local-build-public.der. Run again with -Force to rotate the key
# (running apps built with the old public key then need one manual restart onto a freshly signed build).
param([switch]$Force)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'local_build_signing.ps1')
$publicKeyPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'local-build-public.der'
if ((Get-LocalBuildSigner $publicKeyPath) -and -not $Force) {
    Write-Output "A matching signing key already exists. Public key: $publicKeyPath"
    return
}
$certificate = New-SelfSignedCertificate -Subject $script:LocalBuildSubject -CertStoreLocation Cert:\CurrentUser\My -KeyAlgorithm RSA -KeyLength 3072 -KeyExportPolicy NonExportable -HashAlgorithm SHA256 -KeyUsage DigitalSignature -NotAfter (Get-Date).AddYears(20)
[System.IO.File]::WriteAllBytes($publicKeyPath, (ConvertTo-Spki $certificate))
Write-Output "Created signing key $($certificate.Thumbprint). Public key written to $publicKeyPath"
