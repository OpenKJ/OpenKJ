param([string]$BundleDirectory = 'dist/OpenKJ-Windows-x64-preview')
$ErrorActionPreference = 'Stop'
Add-Type @'
using System.Runtime.InteropServices;
public static class OpenKJBundleErrorMode {
    [DllImport("kernel32.dll")]
    public static extern uint SetErrorMode(uint mode);
}
'@
[void][OpenKJBundleErrorMode]::SetErrorMode(0x8003)
$bundle = (Resolve-Path $BundleDirectory).Path
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('OpenKJ-bundle-check-' + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $temporary | Out-Null
try {
    # Move away from the build tree and remove all installed SDK paths. Force
    # a new registry so a cached system plugin cannot hide a missing bundle DLL.
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    Get-ChildItem Env: | Where-Object { $_.Name -match '^(GST_|QT_PLUGIN_PATH$|QT_QPA_PLATFORM_PLUGIN_PATH$|QML)' } |
        ForEach-Object { Remove-Item "Env:$($_.Name)" }
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_PLUGIN_PATH = $bundle
    $env:GST_REGISTRY_1_0 = Join-Path $temporary 'registry.bin'
    Push-Location $temporary
    try {
        $process = Start-Process -FilePath (Join-Path $bundle 'openkj-runtime-smoke.exe') -PassThru -NoNewWindow
        if (-not $process.WaitForExit(60000)) {
            $process.Kill()
            throw 'Bundled runtime check timed out'
        }
        if ($process.ExitCode -ne 0) { throw "Bundled runtime check failed: $($process.ExitCode)" }
    } finally { Pop-Location }
} finally {
    Remove-Item -Recurse -Force $temporary
}
