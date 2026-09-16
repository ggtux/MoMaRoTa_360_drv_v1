# Run in a normal PowerShell window. No administrator rights required to build.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$isolatedConfig = Join-Path ([IO.Path]::GetTempPath()) ('AstroOrbit-' + [Guid]::NewGuid().ToString('N') + '.NuGet.Config')
Push-Location $PSScriptRoot
try {
    if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) { throw 'Install the .NET 8 SDK, then open a new PowerShell window.' }
    # --configfile uses only this file, bypassing machine/user NuGet settings.
    # Embed it here so replacing Build.ps1 alone is sufficient.
    $configuration = @'
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <clear />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
  <fallbackPackageFolders><clear /></fallbackPackageFolders>
</configuration>
'@
    [IO.File]::WriteAllText($isolatedConfig, $configuration)
    Write-Host 'Astro Orbit Build v3: isolated NuGet configuration'
    & dotnet --version
    $restoreOptions = @(
        '--configfile', $isolatedConfig, '--force', '--verbosity', 'normal',
        '-p:RestoreAdditionalProjectSources=',
        '-p:RestoreFallbackFolders=',
        '-p:RestoreAdditionalProjectFallbackFolders='
    )
    foreach ($project in @('.\Tests\MoMaRoTa.ProtocolTests.csproj', '.\Driver\MoMaRoTa.Driver.csproj', '.\ClientTest\AstroOrbit.ClientTest.csproj')) {
        & dotnet restore $project @restoreOptions
        if ($LASTEXITCODE -ne 0) { throw "Package restore failed: $project. Please share the complete restore output above." }
    }
    & dotnet run --project '.\Tests\MoMaRoTa.ProtocolTests.csproj' --configuration Release --no-restore
    if ($LASTEXITCODE -ne 0) { throw 'Protocol tests failed.' }
    & dotnet build '.\Driver\MoMaRoTa.Driver.csproj' --configuration Release --no-restore
    if ($LASTEXITCODE -ne 0) { throw 'Driver build failed.' }
    & dotnet build '.\ClientTest\AstroOrbit.ClientTest.csproj' --configuration Release --no-restore
    if ($LASTEXITCODE -ne 0) { throw '.NET 8 COM test client build failed.' }
    $output = Join-Path $PSScriptRoot 'Driver\bin\Release\net48'
    Write-Host "Build successful: $output" -ForegroundColor Green
    Write-Host 'For a shareable setup EXE run Build-Installer.ps1; for manual installation run Register-Driver.ps1 as administrator.'
} finally {
    Pop-Location
    if (Test-Path -LiteralPath $isolatedConfig) { Remove-Item -LiteralPath $isolatedConfig }
}
