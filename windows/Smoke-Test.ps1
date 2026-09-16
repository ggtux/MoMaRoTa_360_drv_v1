# Default: only instantiate the driver and inspect metadata, no USB or motion.
# -Connect opens Setup and reads status. -MoveDegrees additionally makes a small move.
param([switch]$Connect, [ValidateRange(-5,5)][double]$MoveDegrees = 0)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($MoveDegrees -ne 0 -and -not $Connect) { throw 'Use -Connect together with -MoveDegrees.' }
# Catch stale DLL registration before a misleading successful PowerShell test.
$views = @([Microsoft.Win32.RegistryView]::Registry32)
if ([Environment]::Is64BitOperatingSystem) { $views += [Microsoft.Win32.RegistryView]::Registry64 }
foreach ($view in $views) {
    $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine, $view)
    $cls = $null; $inproc = $null; $server = $null
    try {
        $cls = $base.OpenSubKey('SOFTWARE\Classes\CLSID\{9BA3AC78-96AF-46D7-9976-E3F940DB8587}')
        if ($null -eq $cls) { throw "Astro Orbit is not registered in $view. Install setup 1.4.2." }
        $inproc = $cls.OpenSubKey('InprocServer32')
        if ($null -ne $inproc) { throw "Old DLL registration in $view. Install setup 1.4.2 with NINA closed." }
        $server = $cls.OpenSubKey('LocalServer32')
        if ($null -eq $server) { throw "Missing LocalServer32 in $view." }
        $serverPath = [string]$server.GetValue('ServerExecutable')
        if (-not (Test-Path -LiteralPath $serverPath)) { throw "Missing server: $serverPath" }
        Write-Host "${view}: $serverPath"
    } finally {
        if ($null -ne $server) { $server.Dispose() }
        if ($null -ne $inproc) { $inproc.Dispose() }
        if ($null -ne $cls) { $cls.Dispose() }
        $base.Dispose()
    }
}
$driver = $null
try {
    $driver = New-Object -ComObject 'ASCOM.MoMaRoTa.Rotator'
    Write-Host "Name: $($driver.Name), driver $($driver.DriverVersion), interface $($driver.InterfaceVersion)"
    if (-not [Runtime.InteropServices.Marshal]::IsComObject($driver)) { throw 'Expected an out-of-process COM wrapper.' }
    if ($driver.DriverVersion -ne '1.4.2') { throw 'Old driver is still active. Close all Astro software, install 1.4.2 and retry.' }
    if ($driver.InterfaceVersion -ne 3) { throw 'Unexpected driver interface version.' }
    if ($Connect) {
        $driver.SetupDialog()
        $driver.Connected = $true
        Write-Host "Position: $($driver.Position), mechanical: $($driver.MechanicalPosition), moving: $($driver.IsMoving)"
        if ($MoveDegrees -ne 0) {
            $driver.Move([single]$MoveDegrees)
            $watch = [Diagnostics.Stopwatch]::StartNew()
            while ($driver.IsMoving) {
                if ($watch.Elapsed.TotalSeconds -gt 25) { $driver.Halt(); throw 'Movement timeout.' }
                Start-Sleep -Milliseconds 250
            }
            Write-Host "Move finished. Position: $($driver.Position), target: $($driver.TargetPosition)"
        }
    }
    Write-Host 'Smoke test passed.' -ForegroundColor Green
} finally {
    if ($null -ne $driver) {
        try { if ($driver.Connected) { $driver.Connected = $false } }
        finally {
            # A .NET Framework COM class can be returned as its managed object.
            # FinalReleaseComObject is valid only for a real COM wrapper (RCW).
            if ([Runtime.InteropServices.Marshal]::IsComObject($driver)) {
                [Runtime.InteropServices.Marshal]::FinalReleaseComObject($driver) | Out-Null
            } elseif ($driver -is [IDisposable]) {
                $driver.Dispose()
            }
            $driver = $null
        }
    }
}
