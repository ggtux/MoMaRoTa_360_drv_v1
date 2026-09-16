# Astro Orbit USB + ASCOM — Windows-Anleitung

## Stand

Die Firmware unterstützt jetzt zusätzlich zum vorhandenen WLAN/Alpaca eine
USB-Seriell-Verbindung mit 115200 Baud. Der C#-Treiber heißt im ASCOM-Auswahldialog
**Astro Orbit** und implementiert `IRotatorV3`.

Der Quellcode liegt in `windows/Driver`. Ab Version **1.3.2** läuft der Treiber
als eigener .NET-Framework-4.8-Prozess `AstroOrbit.LocalServer.exe` (COM LocalServer).
Der Installer registriert diese EXE für 32- und 64-Bit-ASCOM-Anwendungen.
NINA lädt dadurch keine Treiber-DLL mehr in seine eigene .NET-Laufzeit.

**Geprüft auf dem Mac:** Firmware-Build, Protokolltests und C#-Kompilierung.
Die LocalServer-Version muss noch auf Windows mit NINA und Hardware geprüft werden.
Der USB-Verbindungs- und Lesetest der vorherigen Version 1.1 war auf Windows erfolgreich.

### Update auf 1.4.2

NINA dokumentiert den Fehler `System.Runtime, Version=6.0.0.0` bei älteren
In-Process-Treibern: [NINA – ASCOM Connection Issues](https://nighttime-imaging.eu/docs/master/site/troubleshooting/ascom_connection_issues/).
Version 1.4.2 verwendet das vollständige Prozess-, Klassenfabrik- und
Referenzzählungsmodell der offiziellen ASCOM-7-LocalServer-Vorlage. Wie die
Vorlage wird die EXE als signierte x86-Assembly gebaut; als separater Prozess
bleibt sie für 32- und 64-Bit-ASCOM-Clients erreichbar.

1. Den vollständigen neuen `windows`-Ordner auf den Windows-PC übernehmen.
2. NINA und andere Astroprogramme schließen. Einen verbliebenen Prozess
   `AstroOrbit.LocalServer.exe` im Task-Manager beenden.
3. `Build-Installer.ps1` wie unten ausführen und die neue Setup-EXE **1.4.2** installieren.
   Die Registrierung ersetzt den alten DLL-Eintrag automatisch.
4. NINA neu starten und **Astro Orbit** auswählen. COM-Port im Setup prüfen.

Für die Übersetzungskorrektur, die dauerhaft gespeicherte mechanische Position
und den Null-Befehl muss die mitgelieferte Firmware ebenfalls geflasht werden.

## Setup-EXE zum Weitergeben erstellen

Auf deinem **Windows-Build-Rechner** zusätzlich zum .NET 8 SDK
[Inno Setup 6.3 oder neuer](https://jrsoftware.org/isdl.php) installieren.
Danach im Unterordner `windows` eine normale PowerShell öffnen:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build-Installer.ps1
```

Das Skript führt den Treiber-Build einschließlich der Protokolltests aus und
kompiliert anschließend den Installer. Ausgabe bei Version 1.4.2:

```text
windows\dist\Astro-Orbit-ASCOM-Setup-1.4.2.exe
```

**Diese einzelne EXE kannst du weitergeben.** Sie enthält Treiber und benötigte
Bibliotheken, prüft .NET Framework 4.8 und die ASCOM-Profile-Registrierung,
registriert beide COM-Architekturen und legt eine Windows-Deinstallation an.
Die Versionsnummer übernimmt sie aus dem C#-Projekt. Mit einer neuen Version
im selben Projekt wird dieselbe Installation aktualisiert.

Falls der Inno-Compiler an einem anderen Ort installiert ist:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build-Installer.ps1 -InnoCompiler "C:\Pfad\ISCC.exe"
```

**Auf dem Empfänger-PC:** ASCOM Platform 7.1+ und .NET Framework 4.8+ müssen
vorhanden sein. Dann Setup-EXE starten, Administratorabfrage bestätigen und
in der Astrosoftware **Astro Orbit** auswählen. COM-Port unter Properties / Setup
festlegen. Die Schritte zum manuellen Registrieren weiter unten entfallen.
Empfänger brauchen weder SDK noch Visual Studio noch Inno Setup.
Der Installer prüft das Vorhandensein der Profile-Komponente in beiden Architekturen;
er prüft nicht die genaue ASCOM-Plattformversion.

Firmware und USB-Chip-Treiber installiert das Setup nicht. Der ESP32 muss bereits
die USB-Firmware verwenden. Für die EXE gibt es derzeit keine digitale Signatur;
Windows kann deshalb einen unbekannten Herausgeber anzeigen.

**Prüfstand:** Installer-Quellen und Dateiliste sind vorbereitet und geprüft;
Inno-Kompilierung und Installation sind auf diesem Mac nicht ausgeführt worden.
Vor der Weitergabe den Windows-Build, Installation, Verbindung, kleine Bewegung,
Halt, Aktualisierung und Deinstallation mit der Hardware prüfen. Die bekannte
Sync-Einschränkung der Firmware ist im mitinstallierten README beschrieben.
Bei einem fehlgeschlagenen Update ASCOM Platform prüfen und Setup erneut ausführen,
um die COM-Registrierungen wiederherzustellen.

## 1. Windows vorbereiten

Vorgesehener erster Testrechner: Windows 11 x64.

1. [ASCOM Platform](https://ascom-standards.org/Downloads/Index.htm) installieren,
   Version 7.1 oder neuer. Die Plattform liefert auch die Profile-Komponente,
   die das Registrierungsskript benötigt.
2. Das **.NET 8 SDK für Windows x64** von
   [Microsoft](https://dotnet.microsoft.com/download/dotnet/8.0) installieren.
   Das SDK wählen, nicht nur die Runtime. Visual Studio ist nicht erforderlich.
3. Ein neues PowerShell-Fenster öffnen. `dotnet --list-sdks` muss eine 8.0-Version
   zeigen. .NET Framework 4.8 oder neuer wird für den fertigen Treiberprozess benötigt;
   auf Windows 11 ist das normalerweise vorhanden. Die Referenzdateien zum
   Kompilieren lädt das Projekt automatisch über NuGet.
4. Diesen vollständigen Projektordner auf den PC kopieren, zum Beispiel nach
   `C:\Astro\MoMaRoTa_360_drv_v1`. Ein ZIP zuerst vollständig entpacken.

Der erste Build braucht Internet für die festgelegten NuGet-Pakete.
Windows ARM ist noch nicht geprüft; für den ersten Test x64 verwenden.

## 2. Treiber kompilieren

Eine normale PowerShell öffnen, noch nicht als Administrator:

```powershell
cd C:\Astro\MoMaRoTa_360_drv_v1\windows
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build.ps1
```

`ExecutionPolicy Bypass` gilt nur für diesen Prozess; die systemweite Richtlinie
wird nicht geändert. Das Skript führt zuerst die 15 Protokolltests aus und baut
danach den Treiber. Erwartetes Ende: **Build successful**.

Der Treiberprozess steht danach hier:

```text
windows\Driver\bin\Release\net48\AstroOrbit.LocalServer.exe
```

Die EXE, ihre `.exe.config` und die drei Bibliotheken gehören zusammen. Zum
Weitergeben den Installer verwenden.

## 3. Treiber registrieren

Astrosoftware vorher schließen. **Windows PowerShell als Administrator** öffnen:

```powershell
cd C:\Astro\MoMaRoTa_360_drv_v1\windows
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Register-Driver.ps1
```

Die Dateien landen in `C:\Program Files\Astro Orbit USB ASCOM`.
Das Skript ruft die EXE mit `/regserver` auf. Diese registriert beide COM-Architekturen
und ersetzt den alten InprocServer32-Eintrag. Die Dateien bleiben am installierten Ort.

Danach wieder eine **normale Windows PowerShell** öffnen:

```powershell
cd C:\Astro\MoMaRoTa_360_drv_v1\windows
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Smoke-Test.ps1
```

Dieser erste Test lädt nur den COM-Treiber und liest Name/Version. Er öffnet
keinen COM-Port und bewegt den Motor nicht. Erwartet: **Smoke test passed**.

Für einen zusätzlichen 32-Bit-Aktivierungstest:

```powershell
& "$env:WINDIR\SysWOW64\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File .\Smoke-Test.ps1
```

## 4. Neue Firmware aufspielen

Die Firmware lässt sich wie bisher am Mac mit PlatformIO bauen und per USB
hochladen. Im **Firmware-Projektordner**:

```sh
pio run -e esp32dev
pio run -e esp32dev -t upload
```

Falls mehrere Geräte angeschlossen sind, den richtigen Upload-Port explizit
angeben. Alternativ die PlatformIO-Build/Upload-Schaltflächen in VS Code verwenden.
Die bisherige OTA-Umgebung bleibt verfügbar. Ein erfolgreicher Build allein
aktualisiert den ESP32 noch nicht.

USB verbindet den PC mit dem ESP32. Die vorhandene separate Motorversorgung
muss ebenfalls angeschlossen sein; USB allein ersetzt sie nicht.
Nach dem Upload **seriellen Monitor schließen**, bevor der ASCOM-Treiber verbindet.

## 5. USB-Verbindung testen

1. Rotator per Daten-USB-Kabel mit dem Windows-PC verbinden.
2. Im Geräte-Manager unter **Anschlüsse (COM & LPT)** den Port ermitteln.
   Falls kein Port erscheint: Datenkabel und den zum tatsächlich verbauten
   USB-Seriell-Chip passenden Herstellertreiber prüfen.
3. Eine vorhandene Alpaca-Verbindung in der Astrosoftware trennen.
4. In normaler Windows PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Smoke-Test.ps1 -Connect
```

Im Setup den COM-Port wählen und **Save** drücken. Der Test verbindet,
liest Position/Bewegungsstatus und trennt wieder, ohne einen Fahrbefehl zu senden.
Einige ESP32-Boards starten beim Öffnen des Ports trotz deaktiviertem DTR/RTS neu.
Der Treiber wartet beim Verbindungsaufbau bis zu 60 Sekunden auf die Firmware;
ein bestehender WLAN-Verbindungsversuch kann etwa 30 Sekunden dauern.

### Zusätzlich unter .NET 8 testen

Nach dem Build und der Installation aus dem `windows`-Ordner:

```powershell
dotnet run --project .\ClientTest\AstroOrbit.ClientTest.csproj -c Release --no-build -- --connect
```

Das prüft COM-Aufrufe aus .NET 8 mit dem bereits gespeicherten COM-Port, ohne
Fahrbefehl. Ohne `-- --connect` werden nur Name und Version abgefragt. Dieser Test
ersetzt nicht den abschließenden NINA-Test. Fehler des Treiberprozesses stehen unter
`%LOCALAPPDATA%\Astro Orbit\Logs`.

## 6. Erste kleine Bewegung

Den Kabelweg frei halten und mit einer kleinen Bewegung beginnen:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Smoke-Test.ps1 -Connect -MoveDegrees 5
```

Dieser Aufruf bewegt den Rotator tatsächlich um +5°. Das Skript akzeptiert nur
Werte zwischen -5 und +5 Grad. Es wartet auf `IsMoving = false` und liest danach
Position und Ziel. Stopp und Drehrichtung anschließend auch in der Astrosoftware
prüfen. Ein Software-Stopp kann einen elektrisch nicht mehr erreichbaren Motor
nicht garantiert anhalten.

## 7. In der Astrosoftware verwenden

Als ASCOM-Rotator **Astro Orbit** auswählen, unter **Properties / Setup**
den COM-Port speichern und verbinden. Der serielle Monitor und andere USB-Clients
müssen geschlossen sein. Auch der LocalServer unterstützt aktuell nur eine aktive
USB-Client-Verbindung zur Zeit; er teilt die serielle Sitzung nicht zwischen Anwendungen.

Der Treiberprozess startet bei Bedarf automatisch. Wenn der letzte COM-Client sein
Objekt freigibt, beendet ihn das ASCOM-Referenzzählungsmodell automatisch.

Verfügbare Funktionen: Position, MechanicalPosition, TargetPosition, IsMoving,
Move, MoveAbsolute, MoveMechanical, Sync, Halt und Reverse.
Eine Bewegung läuft asynchron; über `IsMoving` auf ihr Ende warten. Ein weiterer
Fahrbefehl, Sync oder Reverse während einer Bewegung wird abgewiesen; zuerst Halt.

### Nullpunkt und Sync

Die bestehende Mode-3-Firmware hat **keinen referenzierten absoluten Nullpunkt**.
Beim ESP32-Neustart ist die aktuelle Stellung wieder der virtuelle mechanische
Nullpunkt. Deshalb nach jedem Neustart neu plate-solven und `Sync` durchführen.
USB und Alpaca teilen sich denselben Sync-Versatz; ein normaler USB-Reconnect ohne
ESP32-Neustart löscht ihn nicht. Der Treiber speichert Reverse auf dem Windows-PC.

**Bekannte ASCOM-Einschränkung:** Der Sync-Versatz wird über einen Controller-Neustart
hinweg nicht erhalten. ASCOM V3 sieht Persistenz vor. Ein alter Offset würde mit dem
neu gesetzten virtuellen Nullpunkt eine falsche Himmelsposition ergeben. Diese
Version übernimmt bewusst die bestehende Firmware-Eigenschaft und ist noch nicht
als vollständig konform geprüft. Für persistente Himmelskoordinaten braucht es ein
verlässliches Referenzierungs-/Positionskonzept, nicht nur das Speichern einer Zahl.

Relative Befehle behalten jetzt ausdrücklich Richtung und volle Umdrehungen bei:
`Move(270)` fährt +270°, nicht den kürzeren Weg -90°. Die bestehende
Kabelbegrenzung (±360° ab Einschaltstellung) und die bisherige
Positions-/Stopp-Erkennung werden weiterverwendet. Die bisherigen Toleranzen können
kleine Restabweichungen auf den Sollwinkel setzen; daraus entsteht keine zusätzliche
mechanische Genauigkeit. Diese Eigenschaften im Hardwaretest berücksichtigen.
Die Kabelposition wird zusätzlich unabhängig von Reverse mitgeführt. Nach einem
Richtungswechsel nahe einer Kabelgrenze kann ein Ziel deshalb abgewiesen werden;
zuerst mit der bisherigen Richtung zurückdrehen. Ein Software-Nullsetzen setzt
diesen Kabelzähler nicht zurück, ein Controller-Neustart dagegen schon.

### USB / WLAN und Verbindungsverlust

- Eine aktive Alpaca-Verbindung verhindert das Übernehmen durch USB.
- Während USB verbunden ist, werden konkurrierende Alpaca-Schreibbefehle und
  Web-Fahrbefehle abgewiesen. Lesen und **Halt / Web-Stopp** bleiben möglich.
- Der Treiber sendet alle zwei Sekunden Statusabfragen als Lebenszeichen.
- Nach mehr als 15 Sekunden ohne gültige USB-Sitzungsanfrage gibt die Firmware
  die USB-Steuerung frei und versucht eine laufende Bewegung zu stoppen.
- Nach einem Absturz kann die alte Sitzung daher bis zu 15 Sekunden blockieren.
- Ein erkannter Neustart, Timeout oder Portfehler führt zum Verbindungsfehler;
  der Treiber sendet einen Fahrbefehl **nicht automatisch erneut**.
- Stall, fehlende Motorantwort oder Bewegungstimeout erscheinen als Treiberfehler,
  statt durch `IsMoving = false` einen erfolgreichen Abschluss vorzutäuschen.

## Aktualisieren / Entfernen

Zum Aktualisieren alle Astroprogramme schließen, erneut `Build.ps1` ausführen und
`Register-Driver.ps1` als Administrator starten.

Zum Entfernen der Registrierung:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Register-Driver.ps1 -Unregister
```

Installationsdateien und Benutzereinstellungen bleiben erhalten.

## Wenn etwas scheitert

- **dotnet nicht gefunden:** SDK installieren und ein neues Terminal öffnen.
- **Class not registered:** Registrierung in der richtigen Architektur prüfen;
  die LocalServer-Registrierung muss in beiden Ansichten vorhanden sein. ASCOM Platform gegebenenfalls reparieren.
- **Zugriff auf COM-Port verweigert:** seriellen Monitor / zweite Anwendung schließen.
- **Keine Antwort:** richtige Firmware, COM-Port und Datenkabel prüfen; bis zu
  60 Sekunden Bootzeit abwarten. Das Protokoll ist erst mit dieser Firmware verfügbar.
- **USB owned / Alpaca connected:** alte Verbindung trennen, nach einem Absturz
  mindestens 15 Sekunden warten und erneut verbinden.
- **Motor feedback unavailable:** separate Motorversorgung und Servo-Bus prüfen.

Bei einem Fehler die vollständige Fehlermeldung und den betroffenen Schritt
festhalten. Für die abschließende Prüfung den Treiber mit ASCOM Conform testen;
Bewegungstests dabei nur am frei beweglichen, vorbereiteten Rotator ausführen.

## Entwickler-Prüfungen

```sh
pio run -e esp32dev
sh tests/run_host_tests.sh
dotnet run --project windows/Tests/MoMaRoTa.ProtocolTests.csproj -c Release
dotnet build windows/Driver/MoMaRoTa.Driver.csproj -c Release
```

Die C++-Tests kompilieren den echten USB-Transport mit simuliertem Servo und
Arduino/RTOS-Adaptern. Sie prüfen nicht die reale Hardware oder Task-Synchronisierung.
Die C#-Tests verwenden den echten Protokollcode mit einem simulierten seriellen Kanal;
sie prüfen nicht Windows-COM oder USB-Treiber. Details des Nachrichtenformats stehen
in `USB-PROTOCOL.md`.
