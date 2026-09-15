; Compile with Inno Setup 6 on Windows (Build-Installer.ps1).
#ifndef AppVersion
  #define AppVersion "1.1.0"
#endif
#define DriverDir "..\Driver\bin\Release\net48"
#define DriverDll "ASCOM.MoMaRoTa.Rotator.dll"

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
; Register only after every dependency has been installed.
Source: "{#DriverDir}\{#DriverDll}"; DestDir: "{app}"; Flags: ignoreversion; AfterInstall: RegisterDriver

[Code]
function RunRegAsm(Is64: Boolean; Unregister: Boolean): Boolean;
var
  Tool, Params: String;
  ExitCode: Integer;
begin
  if Is64 then
    Tool := ExpandConstant('{win}\Microsoft.NET\Framework64\v4.0.30319\RegAsm.exe')
  else
    Tool := ExpandConstant('{win}\Microsoft.NET\Framework\v4.0.30319\RegAsm.exe');
  Params := '"' + ExpandConstant('{app}\{#DriverDll}') + '" /nologo';
  if Unregister then Params := Params + ' /unregister'
  else Params := Params + ' /codebase';
  Result := Exec(Tool, Params, ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ExitCode);
  if Result then Result := ExitCode = 0;
  Log(Format('RegAsm %s %s: exit=%d, success=%d', [Tool, Params, ExitCode, Ord(Result)]));
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
  Registered32, Registered64, Ignored: Boolean;
begin
  Registered32 := RunRegAsm(False, False);
  Registered64 := False;
  if Registered32 then Registered64 := RunRegAsm(True, False);
  if not (Registered32 and Registered64) then begin
    { Remove any partial COM/Chooser registration before file rollback. }
    Ignored := RunRegAsm(True, True);
    Ignored := RunRegAsm(False, True);
    RaiseException('Astro Orbit konnte nicht für beide ASCOM-Architekturen registriert werden. Bitte Astroprogramme schließen, ASCOM Platform prüfen und Setup erneut starten. Details stehen im Setup-Log.');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Removed32, Removed64: Boolean;
begin
  if CurUninstallStep = usUninstall then begin
    Removed32 := RunRegAsm(False, True);
    Removed64 := RunRegAsm(True, True);
    if not (Removed32 and Removed64) then begin
      Log('WARNING: One or more COM registrations could not be removed.');
      MsgBox('Die ASCOM-Registrierung konnte nicht vollständig entfernt werden. Bitte ASCOM Platform reparieren, Astro Orbit erneut installieren und danach deinstallieren.', mbError, MB_OK);
    end;
  end;
end;
