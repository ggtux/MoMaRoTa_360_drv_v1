# Requires Windows PowerShell 5.1, ASCOM Platform 7.1+, administrator rights.
# Registers both 32-bit and 64-bit clients on x64 Windows.
param([switch]$Unregister)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Open Windows PowerShell as administrator and run this script again.'
}
$programFilesRoot = if ($env:ProgramW6432) { $env:ProgramW6432 } else { $env:ProgramFiles }
$install = Join-Path $programFilesRoot 'Astro Orbit USB ASCOM'
$dll = Join-Path $install 'ASCOM.MoMaRoTa.Rotator.dll'
$regasms = @((Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\RegAsm.exe'))
if ([Environment]::Is64BitOperatingSystem) {
    $regasms += (Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\RegAsm.exe')
}
foreach ($regasm in $regasms) { if (-not (Test-Path -LiteralPath $regasm)) { throw "Missing .NET Framework registration tool: $regasm" } }
if ($Unregister) {
    if (-not (Test-Path -LiteralPath $dll)) { throw "Installed driver not found: $dll" }
    foreach ($regasm in $regasms) {
        & $regasm $dll /unregister /nologo
        if ($LASTEXITCODE -ne 0) { throw "Unregistration failed: $regasm" }
    }
    Write-Host "Driver unregistered. The files remain in $install."
    exit 0
}
if (-not [Type]::GetTypeFromProgID('ASCOM.Utilities.Profile')) { throw 'Install or repair ASCOM Platform 7.1+ first.' }
$source = Join-Path $PSScriptRoot 'Driver\bin\Release\net48'
if (-not (Test-Path -LiteralPath (Join-Path $source 'ASCOM.MoMaRoTa.Rotator.dll'))) { throw 'Run Build.ps1 first.' }
New-Item -ItemType Directory -Force -Path $install | Out-Null
# Copy dependencies too; the registered assembly must stay at this fixed path.
Get-ChildItem -LiteralPath $source -File | Where-Object { $_.Extension -in '.dll', '.pdb' } | Copy-Item -Destination $install -Force
foreach ($regasm in $regasms) {
    & $regasm $dll /codebase /nologo
    if ($LASTEXITCODE -ne 0) { throw "Registration failed: $regasm. Close all Astro software and check the ASCOM installation. Re-run this script after fixing the cause." }
}
Write-Host 'Registered: Astro Orbit (ASCOM Chooser)' -ForegroundColor Green
Write-Host 'Run Smoke-Test.ps1 in a normal Windows PowerShell window to test COM activation.'
