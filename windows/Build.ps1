# Run in a normal PowerShell window. No administrator rights required to build.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Push-Location $PSScriptRoot
try {
    if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) { throw 'Install the .NET 8 SDK, then open a new PowerShell window.' }
    & dotnet run --project '.\Tests\MoMaRoTa.ProtocolTests.csproj' --configuration Release
    if ($LASTEXITCODE -ne 0) { throw 'Protocol tests failed.' }
    & dotnet build '.\Driver\MoMaRoTa.Driver.csproj' --configuration Release
    if ($LASTEXITCODE -ne 0) { throw 'Driver build failed.' }
    $output = Join-Path $PSScriptRoot 'Driver\bin\Release\net48'
    Write-Host "Build successful: $output" -ForegroundColor Green
    Write-Host 'Next: open Windows PowerShell as administrator and run Register-Driver.ps1.'
} finally { Pop-Location }
