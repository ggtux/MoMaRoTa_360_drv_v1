; Compile with Inno Setup 6 on Windows (Build-Installer.ps1).
#ifndef AppVersion
  #define AppVersion "1.4.2"
#endif
#define DriverDir "..\Driver\bin\Release\net48"
#define DriverExe "AstroOrbit.LocalServer.exe"

[Setup]
AppId={{D3129EF7-695E-45B9-87BA-FEC80E04A83E}
AppName=Astro Orbit
AppVersion={#AppVersion}
AppVerName=Astro Orbit USB ASCOM {#AppVersion}
DefaultDirName={autopf}\Astro Orbit USB ASCOM
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0
OutputDir=..\dist
OutputBaseFilename=Astro-Orbit-ASCOM-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Astro Orbit USB ASCOM
UninstallDisplayIcon={sys}\shell32.dll,2
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
VersionInfoProductName=Astro Orbit
VersionInfoDescription=Astro Orbit USB ASCOM driver installer
VersionInfoVersion={#AppVersion}

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Explicit list avoids accidentally shipping unrelated build artifacts.
Source: "{#DriverDir}\ASCOM.DeviceInterfaces.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#DriverDir}\ASCOM.Exceptions.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#DriverDir}\Newtonsoft.Json.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "THIRD-PARTY-NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#DriverDir}\{#DriverExe}.config"; DestDir: "{app}"; Flags: ignoreversion
; Register only after every dependency has been installed.
Source: "{#DriverDir}\{#DriverExe}"; DestDir: "{app}"; Flags: ignoreversion; AfterInstall: RegisterDriver

[Code]
function RunServer(Unregister: Boolean): Boolean;
var
  Tool, Params: String;
  ExitCode: Integer;
begin
  Tool := ExpandConstant('{app}\{#DriverExe}');
  if Unregister then Params := '/unregserver'
  else Params := '/regserver';
  ExitCode := -1;
  Result := Exec(Tool, Params, ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ExitCode);
  if Result then Result := ExitCode = 0;
  Log(Format('LocalServer %s: exit=%d, success=%d', [Params, ExitCode, Ord(Result)]));
end;

function InitializeSetup(): Boolean;
var
  FrameworkRelease: Cardinal;
begin
  Result := False;
  if (not RegQueryDWordValue(HKLM32, 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full', 'Release', FrameworkRelease)) or
     (FrameworkRelease < 528040) then begin
    MsgBox('Astro Orbit benötigt .NET Framework 4.8 oder neuer. Bitte zuerst installieren.', mbError, MB_OK);
    Exit;
  end;
  if (not RegKeyExists(HKLM32, 'SOFTWARE\Classes\ASCOM.Utilities.Profile\CLSID')) or
     (not RegKeyExists(HKLM64, 'SOFTWARE\Classes\ASCOM.Utilities.Profile\CLSID')) then begin
    MsgBox('Bitte zuerst ASCOM Platform 7.1 oder neuer installieren bzw. reparieren: https://ascom-standards.org/Downloads/Index.htm', mbError, MB_OK);
    Exit;
  end;
  Result := True;
end;

procedure RegisterDriver;
var
  Ignored: Boolean;
begin
  if not RunServer(False) then begin
    Ignored := RunServer(True);
    RaiseException('Astro Orbit konnte nicht registriert werden. Bitte Astroprogramme schließen und ASCOM Platform prüfen. Details: %LOCALAPPDATA%\Astro Orbit\Logs und Setup-Log.');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then begin
    if not RunServer(True) then begin
      Log('WARNING: COM registration could not be removed.');
      MsgBox('Die ASCOM-Registrierung konnte nicht vollständig entfernt werden. Bitte ASCOM Platform reparieren, Astro Orbit erneut installieren und danach deinstallieren.', mbError, MB_OK);
    end;
  end;
end;
