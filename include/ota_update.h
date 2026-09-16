#pragma once

class AsyncWebServer;

// Initialize and service authenticated PlatformIO / VS Code firmware uploads.
void initOTAUpdate();
void processOTAUpdate();

// Browser upload page and HTTP firmware endpoint.
void setupBrowserOTA(AsyncWebServer &server);
