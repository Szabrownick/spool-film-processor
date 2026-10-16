// =====================================================================
//  ota.h  -  aktualizacja oprogramowania przez WiFi
//  Urzadzenie tworzy siec "FilmProcessor", strona: http://192.168.4.1
//  Plik .bin: Arduino IDE -> Sketch -> Export Compiled Binary
// =====================================================================
#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "config.h"

WebServer otaServer(80);
bool otaActive = false;
int otaProgress = -1;          // -1 brak, 0..100 w trakcie, 101 = OK, 102 = blad

static const char OTA_PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Film Processor</title>
<style>body{font-family:sans-serif;background:#111;color:#fc3;max-width:480px;margin:40px auto;padding:0 16px}
input,button{font-size:18px;margin:10px 0;width:100%}button{background:#d33;color:#fff;border:0;padding:12px;border-radius:8px}</style>
</head><body><h2>Film Processor - aktualizacja</h2>
<form method="POST" action="/update" enctype="multipart/form-data">
<input type="file" name="fw" accept=".bin"><button>Wgraj firmware</button></form>
<p>Wybierz plik <b>.bin</b> (Sketch &rarr; Export Compiled Binary). Po wgraniu urzadzenie zrestartuje sie.</p>
</body></html>)HTML";

void otaStart() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(OTA_SSID, OTA_PASS);
  otaServer.on("/", HTTP_GET, []() { otaServer.send(200, "text/html", OTA_PAGE); });
  otaServer.on("/update", HTTP_POST,
    []() {
      bool ok = !Update.hasError();
      otaServer.send(200, "text/plain", ok ? "OK - restart..." : "BLAD aktualizacji");
      otaProgress = ok ? 101 : 102;
      if (ok) { delay(800); ESP.restart(); }
    },
    []() {
      HTTPUpload &up = otaServer.upload();
      if (up.status == UPLOAD_FILE_START) {
        otaProgress = 0;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
        otaProgress = min(99, (int)(up.totalSize / 15000));   // przyblizony postep
      } else if (up.status == UPLOAD_FILE_END) {
        if (!Update.end(true)) Update.printError(Serial);
      }
    });
  otaServer.begin();
  otaActive = true;
}

void otaStop() {
  if (!otaActive) return;
  otaServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  otaActive = false;
  otaProgress = -1;
}

void otaLoop() { if (otaActive) otaServer.handleClient(); }
