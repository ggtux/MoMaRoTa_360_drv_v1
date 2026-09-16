Astro Orbit 1.4.2 - USB ASCOM Rotator

Voraussetzungen auf dem Ziel-PC:
- Windows x64 (erster Hardwaretest vorgesehen auf Windows 11 x64)
- ASCOM Platform 7.1 oder neuer: https://ascom-standards.org/Downloads/Index.htm
- .NET Framework 4.8 oder neuer
- Astro-Orbit-Firmware mit USB-Protokoll 1 auf dem ESP32
- USB-Datenkabel, passender USB-Seriell-Treiber und separate Motorversorgung

Installation:
Astro-Orbit-ASCOM-Setup ausführen und die Administratorabfrage bestätigen.
Der Installer installiert AstroOrbit.LocalServer.exe und die Bibliotheken.
Er ersetzt alte DLL-Registrierungen durch den eigenen Treiberprozess für 32- und
64-Bit-ASCOM. Dadurch läuft der Treiber unabhängig von NINAs .NET-Laufzeit.
Ein SDK, Visual Studio oder Inno Setup wird auf dem Ziel-PC nicht benötigt.

Verwendung:
In der Astrosoftware den ASCOM-Rotator "Astro Orbit" wählen. In Properties / Setup
den COM-Port speichern. Seriellen Monitor und andere USB-Anwendungen schließen.
Eine bestehende Alpaca-Verbindung zuerst trennen.

Nach einem ESP32-Neustart neu plate-solven und Sync durchführen: Der vorhandene
virtuelle mechanische Nullpunkt wird bei jedem Neustart zurückgesetzt. Der
Sync-Versatz ist nicht über Neustarts persistent. Diese Entwicklungsversion ist
noch nicht vollständig auf ASCOM-Konformität und Hardware geprüft.

Vor Weitergabe bitte Installation, Verbindung, kleine Bewegung, Halt, Aktualisierung
und Deinstallation auf Windows mit der Hardware prüfen.

Deinstallation: Windows Einstellungen > Apps > Astro Orbit USB ASCOM.
COM-Port und Reverse-Einstellungen des Benutzers bleiben dabei erhalten.
Astrosoftware vor Updates oder Deinstallation schließen. Ein verbliebener
AstroOrbit.LocalServer-Prozess kann im Task-Manager beendet werden.
Fehlerprotokoll: %LOCALAPPDATA%\Astro Orbit\Logs
Aktuell ist eine aktive USB-Client-Verbindung gleichzeitig möglich.

Dieses Setup installiert ausschließlich den PC-Treiber. Es flasht keine Firmware
und installiert weder ASCOM Platform noch USB-Chip-Treiber automatisch.
