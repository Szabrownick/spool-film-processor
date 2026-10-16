// =====================================================================
//  touch.h  -  dotyk pojemnosciowy z AUTOWYKRYWANIEM pinow i ukladu
//
//  Producent plytki nie podaje pinow dotyku. Kontroler siedzi na
//  magistrali I2C podpietej do ktoregos z wolnych GPIO (1..5).
//  Skanujemy wszystkie pary SDA/SCL i znane adresy kontrolerow:
//    0x15 CST816 / CST716      (protokol "FT")
//    0x38 FT6336 / FT6206      (protokol "FT")
//    0x5D, 0x14 GT911          (protokol "GT")
//  Znalezione piny zapisujemy w pamieci, wiec kolejny start jest szybki.
// =====================================================================
#pragma once
#include <Wire.h>
#include "config.h"

enum TouchType : uint8_t { TT_NONE = 0, TT_FT = 1, TT_GT = 2 };

struct TouchHw {
  TouchType type = TT_NONE;
  uint8_t   addr = 0;
  int8_t    sda = -1, scl = -1;
} touchHw;

static const int8_t TOUCH_PINS[] = {4, 5, 2, 3, 1};   // kolejnosc = prawdopodobienstwo
static const uint8_t TOUCH_ADDRS[] = {0x15, 0x38, 0x5D, 0x14};

static bool i2cPing(uint8_t a) {
  Wire.beginTransmission(a);
  return Wire.endTransmission() == 0;
}

static bool touchTryPins(int sda, int scl) {
  Wire.end();
  if (!Wire.begin(sda, scl, 100000)) return false;
  Wire.setTimeOut(20);
  for (uint8_t a : TOUCH_ADDRS) {
    if (i2cPing(a)) {
      touchHw.addr = a;
      touchHw.sda = sda;
      touchHw.scl = scl;
      touchHw.type = (a == 0x5D || a == 0x14) ? TT_GT : TT_FT;
      return true;
    }
  }
  Wire.end();
  return false;
}

static bool touchWriteReg8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(touchHw.addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

// Po znalezieniu kontrolera: wylacz auto-usypianie CST816 (inaczej
// po ~2 s bez dotyku przestaje odpowiadac na I2C)
static void touchPostInit() {
  if (touchHw.addr == 0x15) touchWriteReg8(0xFE, 0x01);
  Serial.printf("[TOUCH] znaleziono: adres 0x%02X, SDA=GPIO%d, SCL=GPIO%d, typ=%s\n",
                touchHw.addr, touchHw.sda, touchHw.scl, touchHw.type == TT_GT ? "GT911" : "CST816/FT6x36");
}

// prefSda/prefScl - piny zapamietane z poprzedniego uruchomienia (-1 = brak)
bool touchScan(int8_t prefSda, int8_t prefScl) {
  touchHw = TouchHw();
  // Podciagnij wszystkie kandydujace piny - jesli ktorys to RESET
  // kontrolera, wyprowadzi go to ze stanu resetu.
  for (int8_t p : TOUCH_PINS) pinMode(p, INPUT_PULLUP);
  delay(50);

  if (PIN_TOUCH_SDA >= 0 && PIN_TOUCH_SCL >= 0) {
    if (touchTryPins(PIN_TOUCH_SDA, PIN_TOUCH_SCL)) { touchPostInit(); return true; }
    return false;
  }
  if (prefSda >= 0 && prefScl >= 0 && touchTryPins(prefSda, prefScl)) { touchPostInit(); return true; }

  for (int8_t a : TOUCH_PINS)
    for (int8_t b : TOUCH_PINS)
      if (a != b && touchTryPins(a, b)) { touchPostInit(); return true; }
  return false;
}

// ---------------------------------------------------------------------
//  Odczyt surowych wspolrzednych (uklad wspolrzednych kontrolera)
// ---------------------------------------------------------------------
static bool touchReadRaw(int16_t &rx, int16_t &ry) {
  if (touchHw.type == TT_FT) {
    uint8_t b[5];
    Wire.beginTransmission(touchHw.addr);
    Wire.write(0x02);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom((int)touchHw.addr, 5) != 5) return false;
    for (int i = 0; i < 5; i++) b[i] = Wire.read();
    uint8_t n = b[0] & 0x0F;
    if (n == 0 || n > 5) return false;
    rx = ((b[1] & 0x0F) << 8) | b[2];
    ry = ((b[3] & 0x0F) << 8) | b[4];
    return true;
  }
  if (touchHw.type == TT_GT) {
    static bool lastDown = false;
    static int16_t lx = 0, ly = 0;
    uint8_t st = 0;
    Wire.beginTransmission(touchHw.addr);
    Wire.write(0x81); Wire.write(0x4E);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom((int)touchHw.addr, 1) != 1) return false;
    st = Wire.read();
    if (!(st & 0x80)) { rx = lx; ry = ly; return lastDown; }   // brak nowych danych
    uint8_t n = st & 0x0F;
    if (n > 0) {
      uint8_t b[4];
      Wire.beginTransmission(touchHw.addr);
      Wire.write(0x81); Wire.write(0x50);
      Wire.endTransmission();
      if (Wire.requestFrom((int)touchHw.addr, 4) == 4) {
        for (int i = 0; i < 4; i++) b[i] = Wire.read();
        lx = b[0] | (b[1] << 8);
        ly = b[2] | (b[3] << 8);
      }
    }
    Wire.beginTransmission(touchHw.addr);       // skasuj flage gotowosci
    Wire.write(0x81); Wire.write(0x4E); Wire.write(0x00);
    Wire.endTransmission();
    lastDown = n > 0;
    rx = lx; ry = ly;
    return lastDown;
  }
  return false;
}

// ---------------------------------------------------------------------
//  Kalibracja: przeliczenie surowych -> ekranowych (320x240 poziomo)
//    srcX = swap ? ry : rx ;  x = ax + bx*srcX
//    srcY = swap ? rx : ry ;  y = ay + by*srcY
//  Kalibracja zapisywana jest dla ekranu NIEobroconego. Przy obrocie
//  o 180 st. wynik jest odbijany (x -> 319-x, y -> 239-y).
// ---------------------------------------------------------------------
struct TouchCal {
  uint8_t ok = 0;
  uint8_t swap = 1;
  float ax = 0, bx = 1, ay = 239, by = -1;
};

bool touchGet(const TouchCal &c, bool flipped, int16_t &x, int16_t &y, int16_t *rawX = nullptr, int16_t *rawY = nullptr) {
  int16_t rx, ry;
  if (!touchReadRaw(rx, ry)) return false;
  if (rawX) *rawX = rx;
  if (rawY) *rawY = ry;
  float sx = c.swap ? ry : rx;
  float sy = c.swap ? rx : ry;
  float fx = c.ax + c.bx * sx;
  float fy = c.ay + c.by * sy;
  if (flipped) { fx = 319 - fx; fy = 239 - fy; }
  x = (int16_t)constrain(fx, 0, 319);
  y = (int16_t)constrain(fy, 0, 239);
  return true;
}

// Liczy kalibracje z 3 punktow: P0 (lewy-gora), P1 (prawy-gora), P2 (lewy-dol)
// sx[], sy[] = wspolrzedne ekranowe celow, rx[], ry[] = odczyty surowe
bool touchComputeCal(TouchCal &c, bool flipped, const int16_t sx[3], const int16_t sy[3],
                     const int16_t rx[3], const int16_t ry[3]) {
  int dxr = rx[1] - rx[0], dyr = ry[1] - ry[0];   // ruch w poziomie ekranu
  c.swap = abs(dyr) > abs(dxr) ? 1 : 0;
  float s0x = c.swap ? ry[0] : rx[0], s1x = c.swap ? ry[1] : rx[1];
  float s0y = c.swap ? rx[0] : ry[0], s2y = c.swap ? rx[2] : ry[2];
  if (fabsf(s1x - s0x) < 20 || fabsf(s2y - s0y) < 20) return false;
  c.bx = (float)(sx[1] - sx[0]) / (s1x - s0x);
  c.ax = sx[0] - c.bx * s0x;
  c.by = (float)(sy[2] - sy[0]) / (s2y - s0y);
  c.ay = sy[0] - c.by * s0y;
  if (flipped) {        // zapisz w ukladzie nieobroconym
    c.ax = 319 - c.ax; c.bx = -c.bx;
    c.ay = 239 - c.ay; c.by = -c.by;
  }
  c.ok = 1;
  return true;
}
