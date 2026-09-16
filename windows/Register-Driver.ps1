# Windows PowerShell 5.1, ASCOM Platform 7.1+, administrator rights.
param([switch]$Unregister)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Open Windows PowerShell as administrator and run this script again.'
}
if (Get-Process -Name 'AstroOrbit.LocalServer' -ErrorAction SilentlyContinue) {
    throw 'Close all Astro software and end AstroOrbit.LocalServer.exe in Task Manager before updating or unregistering.'
}
$programFilesRoot = if ($env:ProgramW6432) { $env:ProgramW6432 } else { $env:ProgramFiles }
$install = Join-Path $programFilesRoot 'Astro Orbit USB ASCOM'
$server = Join-Path $install 'AstroOrbit.LocalServer.exe'
if ($Unregister) {
    if (-not (Test-Path -LiteralPath $server)) { throw "Installed server not found: $server" }
    $process = Start-Process -FilePath $server -ArgumentList '/unregserver' -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw 'Unregistration failed. See %LOCALAPPDATA%\Astro Orbit\Logs.' }
    Write-Host "Driver unregistered. The files remain in $install."
    exit 0
}
if (-not [Type]::GetTypeFromProgID('ASCOM.Utilities.Profile')) { throw 'Install or repair ASCOM Platform 7.1+ first.' }
$source = Join-Path $PSScriptRoot 'Driver\bin\Release\net48'
$files = @('AstroOrbit.LocalServer.exe', 'AstroOrbit.LocalServer.exe.config',
           'ASCOM.DeviceInterfaces.dll', 'ASCOM.Exceptions.dll', 'Newtonsoft.Json.dll')
foreach ($file in $files) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $file))) { throw "Run Build.ps1 first. Missing: $file" }
}
New-Item -ItemType Directory -Force -Path $install | Out-Null
foreach ($file in $files) { Copy-Item -LiteralPath (Join-Path $source $file) -Destination $install -Force }
# The EXE replaces the old in-process registration in both registry views.
$process = Start-Process -FilePath $server -ArgumentList '/regserver' -Wait -PassThru
if ($process.ExitCode -ne 0) { throw 'Registration failed. See %LOCALAPPDATA%\Astro Orbit\Logs. Repair ASCOM Platform and retry.' }
Write-Host 'Registered: Astro Orbit 1.2 (isolated COM LocalServer)' -ForegroundColor Green
Write-Host 'Run Smoke-Test.ps1 in a normal Windows PowerShell window.'
