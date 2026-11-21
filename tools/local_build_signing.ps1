# Signing helpers for local build updates (dot-source this file). The private key lives only in the
# CurrentUser certificate store (non-exportable); client\local-build-public.der holds the matching public key (SPKI DER).
$script:LocalBuildSubject = 'CN=SNUWCANVAS Local Build Signing'

function Join-DerBytes([byte[][]]$Parts) {
    $list = New-Object System.Collections.Generic.List[byte]
    foreach ($part in $Parts) { $list.AddRange([byte[]]$part) }
    return , $list.ToArray()
}

function New-DerElement([byte]$Tag, [byte[]]$Content) {
    $length = $Content.Length
    if ($length -lt 128) { $lengthBytes = [byte[]]@($length) }
    elseif ($length -lt 256) { $lengthBytes = [byte[]]@(0x81, $length) }
    else { $lengthBytes = [byte[]]@(0x82, ($length -shr 8), ($length -band 255)) }
    return , (Join-DerBytes @([byte[]]@($Tag), $lengthBytes, $Content))
}

# PowerShell 5.1 has no ExportSubjectPublicKeyInfo: wrap the RSAPublicKey in a SubjectPublicKeyInfo by hand.
function ConvertTo-Spki([System.Security.Cryptography.X509Certificates.X509Certificate2]$Certificate) {
    $rsaKey = $Certificate.PublicKey.EncodedKeyValue.RawData
    $algorithm = [byte[]]@(0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00)
    $bitString = New-DerElement 0x03 (Join-DerBytes @([byte[]]@(0x00), $rsaKey))
    return , (New-DerElement 0x30 (Join-DerBytes @($algorithm, $bitString)))
}

# The signing certificate whose public key equals the committed .der file, or $null.
function Get-LocalBuildSigner([string]$PublicKeyPath) {
    if (-not (Test-Path -LiteralPath $PublicKeyPath)) { return $null }
    $expected = [Convert]::ToBase64String([System.IO.File]::ReadAllBytes($PublicKeyPath))
    foreach ($candidate in @(Get-ChildItem -Path Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $script:LocalBuildSubject -and $_.HasPrivateKey })) {
        if ([Convert]::ToBase64String((ConvertTo-Spki $candidate)) -eq $expected) { return $candidate }
    }
    return $null
}

# Envelope = base64(payload bytes) + base64(RSA PKCS#1 v1.5 SHA-256 signature over exactly those bytes).
function New-LocalBuildEnvelope([byte[]]$PayloadBytes, [System.Security.Cryptography.X509Certificates.X509Certificate2]$Certificate) {
    $rsa = [System.Security.Cryptography.X509Certificates.RSACertificateExtensions]::GetRSAPrivateKey($Certificate)
    $signature = $rsa.SignData($PayloadBytes, [System.Security.Cryptography.HashAlgorithmName]::SHA256, [System.Security.Cryptography.RSASignaturePadding]::Pkcs1)
    return (@{ payload = [Convert]::ToBase64String($PayloadBytes); signature = [Convert]::ToBase64String($signature) } | ConvertTo-Json -Compress)
}
