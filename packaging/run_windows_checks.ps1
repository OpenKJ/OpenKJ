param([string]$BuildDirectory = 'build-windows-dev')
$ErrorActionPreference = 'Stop'

# Children inherit this mode: a missing DLL must fail the test, not wait for a
# desktop error dialog on the hosted runner.
Add-Type @'
using System.Runtime.InteropServices;
public static class OpenKJErrorMode {
    [DllImport("kernel32.dll")]
    public static extern uint SetErrorMode(uint mode);
}
'@
[void][OpenKJErrorMode]::SetErrorMode(0x8003)
& ctest --test-dir $BuildDirectory --output-on-failure
exit $LASTEXITCODE
