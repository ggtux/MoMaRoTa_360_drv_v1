# Astro Orbit Firmware Flasher

`index.html` ist eine statische Seite für GitHub Pages. Sie lädt das neueste
GitHub-Release und übergibt `astro-orbit-factory.bin` an ESP Web Tools. USB-Flash
funktioniert in Chrome oder Edge über Web Serial.

## GitHub Pages einschalten

Den Ordner `firmware-flasher` als Inhalt eines Pages-Branches veröffentlichen
oder seinen Inhalt nach `docs` kopieren und in **Settings → Pages** den Ordner
`/docs` auswählen. Die Seite muss über HTTPS oder localhost laufen.

## Benötigte Release-Dateien

- `astro-orbit-factory.bin`: vollständiges Image für USB, Flash-Adresse 0
- `astro-orbit-firmware.bin`: normales Anwendungsimage für OTA
- `SHA256SUMS.txt`: Prüfsummen beider Dateien

Die Dateien erzeugt `scripts/build_firmware_release.sh`.

OTA wird direkt über `http://astro-orbit.local/update` ausgeführt. Der Nutzer
lädt `astro-orbit-firmware.bin` aus dem Release und wählt sie dort aus.
