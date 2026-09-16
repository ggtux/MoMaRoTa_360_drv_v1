#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include "ota_update.h"
#include "display_control.h"
#include "servo_control.h"
#include "wifi_manager.h"

namespace {
const char *OTA_HOSTNAME = "astro-orbit";
bool otaEnabled = false;
unsigned int lastProgressPercent = 101;
bool browserRestartPending = false;
unsigned long browserRestartAt = 0;
bool browserUploadSuccess = false;

const char UPDATE_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Astro Orbit Firmware Update</title><style>
body{font-family:system-ui;background:#101416;color:#eef5f2;max-width:620px;margin:40px auto;padding:20px}
.card{background:#182126;border:1px solid #314047;border-radius:16px;padding:24px}h1{margin-top:0;color:#62d6b2}
input,button{box-sizing:border-box;width:100%;margin-top:14px;padding:12px;border-radius:8px}
button{border:0;background:#287f68;color:white;font-weight:700;cursor:pointer}button:disabled{opacity:.5}
progress{width:100%;height:22px;margin-top:18px}.muted{color:#afbdba;font-size:.92rem}#status{min-height:1.5em}
</style></head><body><div class="card"><h1>Astro Orbit OTA Update</h1>
<p>Die normale <b>astro-orbit-firmware.bin</b> aus dem GitHub-Release auswählen.</p>
<p><a href="https://github.com/ggtux/MoMaRoTa_360_drv_v1/releases/latest" target="_blank" style="color:#62d6b2">Neuestes GitHub-Release öffnen</a></p>
<input id="file" type="file" accept=".bin,application/octet-stream"><button id="go">Firmware hochladen</button>
<progress id="bar" max="100" value="0"></progress><p id="status"></p>
<p class="muted">Während des Updates Strom und WLAN nicht trennen. Der Controller startet danach automatisch neu.</p>
</div><script>
const file=document.querySelector('#file'),go=document.querySelector('#go'),bar=document.querySelector('#bar'),status=document.querySelector('#status');
go.onclick=()=>{if(!file.files.length){status.textContent='Bitte zuerst eine .bin-Datei auswählen.';return}
if(!confirm('Firmware jetzt installieren?'))return;go.disabled=true;const form=new FormData();form.append('firmware',file.files[0]);
const x=new XMLHttpRequest();x.open('POST','/update');x.upload.onprogress=e=>{if(e.lengthComputable){bar.value=e.loaded/e.total*100;status.textContent='Upload: '+Math.round(bar.value)+' %'}};
x.onload=()=>{status.textContent=x.status===200?'Update vollständig. Controller startet neu.':'Update fehlgeschlagen: '+x.responseText;go.disabled=false};
x.onerror=()=>{status.textContent='Verbindung während des Updates beendet. Prüfe nach einigen Sekunden den Neustart.';go.disabled=false};x.send(form)};
</script></body></html>)HTML";
}

void setupBrowserOTA(AsyncWebServer &server) {
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html; charset=utf-8", UPDATE_PAGE);
    });

    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            bool success = browserUploadSuccess && !Update.hasError();
            request->send(success ? 200 : 500, "text/plain",
                          success ? "Firmware accepted; restarting" : "Firmware update failed");
            if(success) {
                browserRestartPending = true;
                browserRestartAt = millis() + 1500;
            }
        },
        [](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {
            if(index == 0) {
                browserUploadSuccess = false;
                stopServo();
                Serial.print("Browser OTA started: ");
                Serial.println(filename);
                displayMessage("OTA Update", "Receiving...");
                if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) Update.printError(Serial);
            }
            if(!Update.hasError() && Update.write(data, len) != len) Update.printError(Serial);
            if(final) {
                if(Update.end(true)) {
                    browserUploadSuccess = true;
                    Serial.printf("Browser OTA complete: %u bytes\n", (unsigned)(index + len));
                    displayMessage("OTA Update", "Complete", "Restarting...");
                } else {
                    Update.printError(Serial);
                    displayMessage("OTA Update", "Error");
                }
            }
        });
}

void initOTAUpdate() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);

    ArduinoOTA.onStart([]() {
        stopServo();
        lastProgressPercent = 101;
        Serial.println("OTA update started");
        displayMessage("OTA Update", "Receiving...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        unsigned int percent = total > 0 ? (progress * 100U) / total : 0;
        if(percent == lastProgressPercent) {
            return;
        }

        lastProgressPercent = percent;
        Serial.printf("OTA progress: %u%%\r", percent);

        if(percent % 10U == 0U) {
            String progressText = String(percent) + "%";
            displayMessage("OTA Update", progressText.c_str());
        }
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA update complete - restarting");
        displayMessage("OTA Update", "Complete", "Restarting...");
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("\nOTA error [%u]: ", error);
        switch(error) {
            case OTA_AUTH_ERROR: Serial.println("authentication failed"); break;
            case OTA_BEGIN_ERROR: Serial.println("begin failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("connection failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("receive failed"); break;
            case OTA_END_ERROR: Serial.println("end failed"); break;
            default: Serial.println("unknown error"); break;
        }

        displayMessage("OTA Update", "Error");
    });

    ArduinoOTA.begin();
    otaEnabled = true;

    Serial.print("OTA ready: ");
    Serial.print(OTA_HOSTNAME);
    Serial.print(".local (");
    Serial.print(getIPAddress());
    Serial.println(")");
}

void processOTAUpdate() {
    if(otaEnabled) {
        ArduinoOTA.handle();
    }
    if(browserRestartPending && (long)(millis() - browserRestartAt) >= 0) {
        ESP.restart();
    }
}
