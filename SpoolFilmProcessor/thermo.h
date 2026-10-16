// =====================================================================
//  thermo.h  -  DS18B20 na GPIO20, odczyt nieblokujacy
// =====================================================================
#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

OneWire oneWire(PIN_DS18B20);
DallasTemperature dallas(&oneWire);

#define THERMO_HIST 150          // historia do wykresu: 150 probek co 2 s = 5 min

class Thermo {
public:
  float t = NAN;                 // temperatura po korekcie [C]
  bool valid = false;
  float offset = 0;              // korekta kalibracyjna [C]
  float tMin = NAN, tMax = NAN;
  float hist[THERMO_HIST];
  uint16_t histN = 0, histPos = 0;

  void begin() {
    dallas.begin();
    dallas.setWaitForConversion(false);
    findSensor();
    request();
  }

  void update() {
    uint32_t now = millis();
    if (!found) {
      if (now - lastSearch > 3000) { findSensor(); if (found) request(); }
      valid = false;
      return;
    }
    if (now - reqMs < 800) return;          // 12 bit = max 750 ms konwersji
    float c = dallas.getTempC(addr);
    if (c == DEVICE_DISCONNECTED_C || c < -50 || c > 110 || c == 85.0f) {
      if (++errors > 3) { valid = false; found = false; }
    } else {
      errors = 0;
      t = c + offset;
      valid = true;
      if (isnan(tMin) || t < tMin) tMin = t;
      if (isnan(tMax) || t > tMax) tMax = t;
      if (now - lastHist >= 2000) {
        lastHist = now;
        hist[histPos] = t;
        histPos = (histPos + 1) % THERMO_HIST;
        if (histN < THERMO_HIST) histN++;
      }
    }
    request();
  }

  void resetMinMax() { tMin = tMax = t; }
  bool sensorFound() const { return found; }
  const uint8_t *address() const { return addr; }

private:
  DeviceAddress addr;
  bool found = false;
  uint8_t errors = 0;
  uint32_t reqMs = 0, lastSearch = 0, lastHist = 0;

  void findSensor() {
    lastSearch = millis();
    // Wewnetrzne podciaganie ESP (~45k) jako zapas, gdy brak rezystora 4.7k.
    // OneWire na ESP32-C3 tylko wylacza wyjscie, wiec podciaganie zostaje aktywne.
    pinMode(PIN_DS18B20, INPUT_PULLUP);
    dallas.begin();
    found = dallas.getAddress(addr, 0);
    if (found) {
      dallas.setResolution(addr, 12);
      Serial.printf("[TEMP] DS18B20 znaleziony: %02X%02X%02X%02X%02X%02X%02X%02X\n",
                    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
    }
  }
  void request() { dallas.requestTemperaturesByAddress(addr); reqMs = millis(); }
};

Thermo thermo;
