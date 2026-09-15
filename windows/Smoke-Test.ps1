# Default: only instantiate the driver and inspect metadata, no USB or motion.
# -Connect opens Setup and reads status. -MoveDegrees additionally makes a small move.
param([switch]$Connect, [ValidateRange(-5,5)][double]$MoveDegrees = 0)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($MoveDegrees -ne 0 -and -not $Connect) { throw 'Use -Connect together with -MoveDegrees.' }
$driver = $null
try {
    $driver = New-Object -ComObject 'ASCOM.MoMaRoTa.Rotator'
    Write-Host "Name: $($driver.Name), driver $($driver.DriverVersion), interface $($driver.InterfaceVersion)"
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
