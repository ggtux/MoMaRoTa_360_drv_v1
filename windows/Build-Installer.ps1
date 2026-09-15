# Creates the shareable setup EXE. Needs .NET 8 SDK and Inno Setup 6.3+.
param([string]$InnoCompiler)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) { throw 'Run this installer build on Windows.' }
if (-not $InnoCompiler) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $InnoCompiler = $command.Source }
    else {
        foreach ($root in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
            if ($root) {
                $candidate = Join-Path $root 'Inno Setup 6\ISCC.exe'
                if (Test-Path -LiteralPath $candidate) { $InnoCompiler = $candidate; break }
            }
        }
    }
}
if (-not $InnoCompiler -or -not (Test-Path -LiteralPath $InnoCompiler)) {
    throw 'Install Inno Setup 6.3+ from https://jrsoftware.org/isdl.php or pass -InnoCompiler with the full path to ISCC.exe.'
}
# A fresh tested driver build is mandatory; never package an old DLL after a failed build.
& (Join-Path $PSScriptRoot 'Build.ps1')
[xml]$project = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Driver\MoMaRoTa.Driver.csproj') -Raw
$version = [string]$project.Project.PropertyGroup.Version
if ($version -notmatch '^\d+\.\d+\.\d+(\.\d+)?$') { throw "Invalid installer version: $version" }
$expected = Join-Path $PSScriptRoot "dist\Astro-Orbit-ASCOM-Setup-$version.exe"
if (Test-Path -LiteralPath $expected) { Remove-Item -LiteralPath $expected }
& $InnoCompiler "/DAppVersion=$version" (Join-Path $PSScriptRoot 'Installer\AstroOrbit.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
if (-not (Test-Path -LiteralPath $expected)) { throw "Installer output missing: $expected" }
Write-Host "Ready to share: $expected" -ForegroundColor Green
Get-FileHash -LiteralPath $expected -Algorithm SHA256 | Format-List
