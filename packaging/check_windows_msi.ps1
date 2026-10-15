param(
    [Parameter(Mandatory)][string]$Installer,
    [Parameter(Mandatory)][string]$PreviousInstaller,
    [string]$BundleDirectory = 'dist/OpenKJ-Windows-x64-preview'
)
$ErrorActionPreference = 'Stop'
$msi = (Resolve-Path $Installer).Path
$previousMsi = (Resolve-Path $PreviousInstaller).Path
$payload = (Resolve-Path $BundleDirectory).Path
$install = Join-Path $env:ProgramW6432 'OpenKJ'
$logs = New-Item -ItemType Directory -Force 'build-windows-msi/logs'
$registryRoot = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall'
$programs = Join-Path ([Environment]::GetFolderPath('CommonPrograms')) 'OpenKJ/OpenKJ.lnk'
$desktop = Join-Path ([Environment]::GetFolderPath('CommonDesktopDirectory')) 'OpenKJ.lnk'

function Get-MsiProperty([string]$Path, [string]$Name) {
    $engine = New-Object -ComObject WindowsInstaller.Installer
    $database = $engine.OpenDatabase($Path, 0)
    $view = $database.OpenView("SELECT ``Value`` FROM ``Property`` WHERE ``Property``='$Name'")
    $view.Execute()
    $record = $view.Fetch()
    if (-not $record) { throw "Missing MSI property: $Name" }
    return $record.StringData(1)
}
function Invoke-Msi([string[]]$Arguments, [string]$LogName, [int[]]$Expected = @(0,3010)) {
    $log = Join-Path $logs.FullName "$LogName.log"
    $process = Start-Process "$env:SystemRoot/System32/msiexec.exe" -ArgumentList ($Arguments + @('/qn','/norestart','/L*v', "`"$log`"")) -PassThru
    if (-not $process.WaitForExit(180000)) { $process.Kill(); throw "MSI operation timed out: $LogName" }
    if ($process.ExitCode -notin $Expected) {
        Get-Content $log -Tail 70
        throw "MSI operation $LogName failed: $($process.ExitCode)"
    }
}
function Assert-Payload {
    $manifest = Get-Content (Join-Path $payload 'manifest.json') -Raw | ConvertFrom-Json
    foreach ($entry in $manifest.files.PSObject.Properties) {
        $path = Join-Path $install $entry.Name
        if (-not (Test-Path -LiteralPath $path)) { throw "Installed file missing: $path" }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.Value.sha256) {
            throw "Installed file differs from tested bundle: $path"
        }
    }
    if (-not (Test-Path $programs)) { throw 'Start Menu shortcut missing' }
    if (-not (Test-Path $desktop)) { throw 'Optional desktop shortcut missing' }
}
$currentCode = Get-MsiProperty $msi 'ProductCode'
$previousCode = Get-MsiProperty $previousMsi 'ProductCode'
$version = Get-MsiProperty $msi 'ProductVersion'
# This script installs machine-wide packages and is intended ONLY for a fresh,
# disposable CI runner. Never remove a developer's existing installation.
if ((Test-Path $install) -or (Test-Path $programs) -or (Test-Path $desktop)) {
    throw 'Use a clean Windows test machine; OpenKJ already exists'
}
$dataDirectory = Join-Path $env:APPDATA 'OpenKJ/OpenKJ'
New-Item -ItemType Directory -Force $dataDirectory | Out-Null
$sentinel = Join-Path $dataDirectory ('msi-preservation-' + [Guid]::NewGuid() + '.txt')
$media = Join-Path $env:RUNNER_TEMP ('msi-media-' + [Guid]::NewGuid() + '.mp3')
[IO.File]::WriteAllText($sentinel, 'Existing user settings and library data must survive.')
[IO.File]::WriteAllText($media, 'Existing media must survive.')
try {
    Invoke-Msi @('/i', "`"$previousMsi`"", 'ADDLOCAL=ALL') 'install-previous'
    Assert-Payload
    if (-not (Test-Path (Join-Path $registryRoot $previousCode))) { throw 'Previous product not registered' }
    Invoke-Msi @('/i', "`"$msi`"", 'ADDLOCAL=ALL') 'upgrade'
    if (Test-Path (Join-Path $registryRoot $previousCode)) { throw 'Upgrade left the previous product registered' }
    if ((Get-ItemProperty (Join-Path $registryRoot $currentCode)).DisplayVersion -ne $version) { throw 'Installed version mismatch' }
    Assert-Payload
    # Validate repair of a required plugin, then test the real installed runtime
    # in a child shell so its isolated PATH cannot affect later MSI operations.
    Remove-Item (Join-Path $install 'sqldrivers/qsqlite.dll')
    Invoke-Msi @('/fa', "`"$msi`"") 'repair'
    Assert-Payload
    & (Join-Path $PSHOME 'pwsh.exe') -NoProfile -File (Join-Path $PSScriptRoot 'check_windows_bundle.ps1') -BundleDirectory $install
    if ($LASTEXITCODE -ne 0) { throw 'Installed runtime smoke test failed' }
    Invoke-Msi @('/i', "`"$previousMsi`"") 'reject-downgrade' @(1603)
    if (-not (Test-Path (Join-Path $registryRoot $currentCode))) { throw 'Downgrade changed the installed product' }
    Invoke-Msi @('/x', $currentCode) 'uninstall'
    if ((Test-Path $install) -or (Test-Path $programs) -or (Test-Path $desktop) -or (Test-Path (Join-Path $registryRoot $currentCode))) {
        throw 'Uninstall left installed files, shortcuts, or product registration'
    }
    if ((Get-Content $sentinel -Raw) -ne 'Existing user settings and library data must survive.' -or
        (Get-Content $media -Raw) -ne 'Existing media must survive.') { throw 'User data or media changed' }
    Write-Host 'PASS: MSI install, payload hashes, shortcuts, upgrade, repair, isolated runtime, downgrade rejection, uninstall, and user-data preservation.'
} finally {
    foreach ($code in @($currentCode, $previousCode)) {
        if (Test-Path (Join-Path $registryRoot $code)) { Invoke-Msi @('/x', $code) "cleanup-$code" }
    }
    Remove-Item -LiteralPath $sentinel, $media -Force -ErrorAction SilentlyContinue
}
