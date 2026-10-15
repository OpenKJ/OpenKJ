param(
    [string]$BundleDirectory = 'dist/OpenKJ-Windows-x64-preview',
    [string]$OutputDirectory = 'dist',
    [string]$Version,
    [string]$Wix = 'build-windows-msi/tools/wix.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$bundle = (Resolve-Path $BundleDirectory).Path
$wixPath = (Resolve-Path $Wix).Path
if (-not $Version) {
    $header = Get-Content (Join-Path $root 'src/okjversion.h') -Raw
    $Version = [regex]::Match($header, 'OKJ_VERSION_STRING "([0-9.]+)"').Groups[1].Value
}
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw "Invalid MSI version: $Version" }
$parts = $Version.Split('.') | ForEach-Object { [int]$_ }
if ($parts[0] -gt 255 -or $parts[1] -gt 255 -or $parts[2] -gt 65535) { throw 'Version exceeds MSI limits' }
if (-not (Test-Path (Join-Path $bundle 'manifest.json'))) { throw 'Stage the complete Windows bundle first' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path $OutputDirectory).Path
New-Item -ItemType Directory -Force -Path 'build-windows-msi' | Out-Null
$license = Join-Path (Resolve-Path 'build-windows-msi').Path 'LICENSE.rtf'
# License is ASCII; escape RTF syntax while preserving the complete GPL text.
$text = [IO.File]::ReadAllText((Join-Path $root 'LICENSE'))
$text = $text.Replace('\', '\\').Replace('{', '\{').Replace('}', '\}').Replace("`r", '').Replace("`n", '\par ')
[IO.File]::WriteAllText($license, '{\rtf1\ansi\deff0 {\fonttbl {\f0 Courier New;}}\f0\fs16 ' + $text + '}', [Text.Encoding]::ASCII)
$msi = Join-Path $output "OpenKJ-$Version-windows-x64.msi"
& $wixPath build (Join-Path $PSScriptRoot 'windows/OpenKJ.wxs') -arch x64 `
    -ext WixToolset.UI.wixext/5.0.2 -d "AppVersion=$Version" -d "PayloadDir=$bundle" `
    -d "RepoDir=$root" -d "LicenseRtf=$license" -o $msi
if ($LASTEXITCODE -ne 0) { throw "WiX failed: $LASTEXITCODE" }
$digest = (Get-FileHash $msi -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$msi.sha256", "$digest  $([IO.Path]::GetFileName($msi))`n")
Write-Host "Built $msi"
