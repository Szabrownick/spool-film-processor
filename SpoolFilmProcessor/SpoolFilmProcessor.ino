// =====================================================================
//  Spool Film Processor  -  procesor do wywolywania filmow (DIY)
//  ESP32-C3 + TFT 2.4" ST7789 z dotykiem (msalamon EVB-00055)
//  TMC2209 (UART, 1 pin) + JK42HS48 + DS18B20 (1 pin)
//
//  Wymagane biblioteki (Library Manager):
//    - LovyanGFX        (lovyan03)
//    - OneWire          (Paul Stoffregen)
//    - DallasTemperature (Miles Burton)
//  Plytka: "ESP32C3 Dev Module", USB CDC On Boot: Enabled,
//          Partition Scheme: "Minimal SPIFFS (1.9MB APP with OTA)"
// =====================================================================
#include "config.h"
#include "display.h"
#include "touch.h"
#include "motor.h"
#include "thermo.h"
#include "bootio.h"
#include "compensation.h"
#include "programs.h"
#include "ota.h"
#include "ui.h"
#include "spool_logo.h"

// ---------------------------------------------------------------------
//  Intro: napis SPOOL "wywolujacy sie" jak zdjecie (2 s)
//   0.00-0.45  negatyw: ciemny napis na pomaranczowej masce filmu C-41
//   0.45-0.55  odwrocenie na pozytyw (blysk)
//   0.55-1.00  wywolywanie: napis z ciemnego bursztynu do pelnej bieli
// ---------------------------------------------------------------------
#define LOGO_X (160 - SPOOL_WORD_W / 2)
#define LOGO_Y 82

uint16_t mix565(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, float t) {
  t = constrain(t, 0.0f, 1.0f);
  return rgb565(r1 + (r2 - r1) * t, g1 + (g2 - g1) * t, b1 + (b2 - b1) * t);
}

void drawIntroFrame(float p) {          // p = 0..1 (postep intro)
  // logo SPOOL od razu biale; podpis "FILM PROCESSOR" lagodnie sie pojawia
  static bool drawn = false;
  if (!drawn || p <= 0.0f) {
    tft.fillScreen(C_BG);
    tft.drawBitmap(LOGO_X, LOGO_Y, SPOOL_WORD, SPOOL_WORD_W, SPOOL_WORD_H, C_WHITE);
    tft.setFont(&lgfx::fonts::Font0);
    txt(&tft, "v" FW_VERSION, 314, 230, 2, C_DIM);
    txt(&tft, TR("dotknij = kalibracja", "tap = calibrate"), 6, 230, 0, C_DIM);
    drawn = true;
  }
  float t = constrain((p - 0.25f) / 0.5f, 0.0f, 1.0f);
  tft.setFont(&lgfx::fonts::Font0);
  txt(&tft, "F I L M   P R O C E S S O R", 160, LOGO_Y + SPOOL_WORD_H + 22, 1, mix565(7, 7, 7, 135, 135, 135, t));
}

void drawIntro() { drawIntroFrame(1.0f); }
// pasek postepu intro (akcent Spool) - 0..1
void drawIntroBarUnused(float p) {
  int w = (int)(200 * constrain(p, 0.0f, 1.0f));
  tft.fillRect(60, 226, 200, 2, C_BORDER);
  tft.fillRect(60, 226, w, 2, C_ACC);
}

void drawSplash(const char *msg) {
  drawIntro();
  tft.fillRect(0, 218, 320, 22, C_BG);
  tft.setFont(F_S);
  txt(&tft, msg, 160, 226, 1, C_TXT2);
}

void handleTouch() {
  static bool was = false;
  static uint32_t downAt = 0, lastRep = 0;
  static bool heldRep = false;
  static uint8_t upCnt = 0;
  static uint32_t lastPress = 0;
  int16_t x = 0, y = 0, rx = 0, ry = 0;
  bool rawDown = touchGet(S.cal, S.flip, x, y, &rx, &ry);
  uint32_t now = millis();

  // debounce: CST816 potrafi na chwile "zgubic" palec -> bez tego jedno
  // dotkniecie dawalo dwa klikniecia. Puszczenie liczy sie dopiero po
  // TOUCH_RELEASE_N pustych odczytach z rzedu.
  bool down;
  if (rawDown) { upCnt = 0; down = true; }
  else { if (upCnt < 255) upCnt++; down = was && upCnt < TOUCH_RELEASE_N; }

  // blokada po zmianie ekranu / obrocie / punkcie kalibracji:
  // nowe dotkniecie dopiero po puszczeniu palca i uplywie czasu
  if (touchLockUntil && (int32_t)(now - touchLockUntil) < 0) { was = down; if (down) heldId = 0; return; }
  if (touchLockUntil && !down) touchLockUntil = 0;

  if (scr == S_CALIB && !calTest) { calibTouch(down, rawDown, rx, ry); was = down; return; }
  if (scr == S_CALIB && calTest && rawDown) tft.fillCircle(x, y, 3, C_OK);

  if (down && !was && now - lastPress < TOUCH_MIN_GAP_MS) { was = down; heldId = 0; return; }
  if (down && !was) {
    lastPress = now;
    int i = hitBtn(x, y);
    if (i >= 0) {
      heldId = btns[i].id;
      heldRep = btns[i].rep;
      downAt = lastRep = now;
      repeatCount = 0;
      BEEP(PAT_CLICK);
      onButton(heldId);
    } else heldId = 0;
  } else if (down && heldId && heldRep && now - downAt > 450 && now - lastRep > 110) {
    lastRep = now;
    repeatCount++;
    onButton(heldId);
  }
  if (!down) heldId = 0;
  was = down;
}

// ---------------------------------------------------------------------
//  Diagnostyka UART TMC2209 (komenda 'u').
//  Na czas testu RXO (GPIO20) podlacz do pinu TX sterownika (DS18B20 odlaczony).
//  Wysylamy zapytanie o rejestr IOIN pod adresy 0..3 i czytamy odpowiedz.
// ---------------------------------------------------------------------
bool tmcRead(uint8_t addr, uint8_t reg, uint32_t &val, int &echo, int &total) {
  uint8_t req[4] = {0x05, addr, reg, 0};
  req[3] = TMC2209Uart::crc(req, 3);
  while (Serial1.available()) Serial1.read();
  Serial1.write(req, 4);
  Serial1.flush();
  uint8_t b[24];
  int n = 0;
  uint32_t t = millis();
  while (millis() - t < 40 && n < 24) if (Serial1.available()) b[n++] = Serial1.read();
  total = n;
  echo = (n >= 4 && memcmp(b, req, 4) == 0) ? 4 : 0;
  for (int i = 0; i + 8 <= n; i++) {
    if (b[i] == 0x05 && b[i + 1] == 0xFF && b[i + 2] == reg && b[i + 7] == TMC2209Uart::crc(b + i, 7)) {
      val = ((uint32_t)b[i + 3] << 24) | ((uint32_t)b[i + 4] << 16) | ((uint32_t)b[i + 5] << 8) | b[i + 6];
      return true;
    }
  }
  return false;
}

void tmcDiag() {
  Serial.println("[UART] Test TMC2209: RXO(GPIO20) musi byc polaczony z pinem TX sterownika");
  Serial1.end();
  Serial1.begin(TMC_BAUD, SERIAL_8N1, PIN_DS18B20, PIN_TMC_UART);
  delay(20);
  bool any = false;
  for (uint8_t a = 0; a < 4; a++) {
    uint32_t v = 0; int echo = 0, total = 0;
    bool ok = tmcRead(a, 0x06, v, echo, total);
    if (ok) {
      any = true;
      Serial.printf("[UART] adres %d: ODPOWIADA, IOIN=0x%08lX, wersja ukladu=0x%02lX %s\n", a, (unsigned long)v,
                    (unsigned long)(v >> 24), (v >> 24) == 0x21 ? "(TMC2209 OK)" : "");
    } else {
      Serial.printf("[UART] adres %d: brak odpowiedzi (odebrano %d bajtow, echo wlasnego zapytania: %s)\n",
                    a, total, echo ? "TAK" : "NIE");
    }
  }
  if (!any) Serial.println("[UART] Wniosek: echo NIE = przewod RXO nie dochodzi do linii PDN/RX/TX; "
                           "echo TAK bez odpowiedzi = sterownik nie slucha (VIO? zly pin? MS1/MS2?)");
  Serial1.end();
  Serial1.begin(TMC_BAUD, SERIAL_8N1, -1, PIN_TMC_UART);
  Serial.println("[UART] koniec testu - po tescie zrob RESET plytki (czujnik temperatury)");
}

// Test bez dodatkowych przewodow (komenda 'w'): UART w trybie jednoprzewodowym
// na GPIO21 (nadawanie i odbior na tym samym pinie, open-drain + podciaganie).
void tmcDiagOneWire() {
  Serial.println("[UART1W] Test TMC2209 przez sam GPIO21 (TXO -> 1k -> RX)");
  Serial1.end();
  Serial1.begin(TMC_BAUD, SERIAL_8N1, PIN_TMC_UART, PIN_TMC_UART);
  gpio_pullup_en((gpio_num_t)PIN_TMC_UART);
  delay(20);
  bool any = false;
  for (uint8_t a = 0; a < 4; a++) {
    uint32_t v = 0; int echo = 0, total = 0;
    bool ok = tmcRead(a, 0x06, v, echo, total);
    if (ok) {
      any = true;
      Serial.printf("[UART1W] adres %d: ODPOWIADA, IOIN=0x%08lX, wersja=0x%02lX %s\n", a, (unsigned long)v,
                    (unsigned long)(v >> 24), (v >> 24) == 0x21 ? "(TMC2209 OK)" : "");
      uint32_t g = 0;
      if (tmcRead(a, 0x00, g, echo, total)) Serial.printf("[UART1W]   GCONF=0x%08lX\n", (unsigned long)g);
      if (tmcRead(a, 0x02, g, echo, total)) Serial.printf("[UART1W]   licznik zapisow IFCNT=%lu\n", (unsigned long)(g & 0xFF));
    } else {
      Serial.printf("[UART1W] adres %d: brak odpowiedzi (odebrano %d B, echo: %s)\n", a, total, echo ? "TAK" : "NIE");
    }
  }
  if (!any) Serial.println("[UART1W] Wniosek: echo TAK + brak odpowiedzi = sterownik nie slucha (VIO, MS1/MS2, pin RX)");
  Serial1.end();
  Serial1.begin(TMC_BAUD, SERIAL_8N1, -1, PIN_TMC_UART);
  Serial.println("[UART1W] koniec testu");
}

// Komendy testowe z Serial Monitor (115200): b = pik, a = alarm, f = melodia,
// t = temperatura, i = informacje
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'b': bootio.soundOn = true; BEEP(PAT_STEP);   Serial.println("[TEST] pik"); break;
      case 'h': bootio.soundOn = true; BEEP(PAT_HOT);    Serial.println("[TEST] woda za goraca"); break;
      case 'c': bootio.soundOn = true; BEEP(PAT_COLD);   Serial.println("[TEST] woda za zimna"); break;
      case 'o': bootio.soundOn = true; BEEP(PAT_STEP);   Serial.println("[TEST] nastepny krok"); break;
      case 'a': bootio.soundOn = true; BEEP(PAT_ALARM);  Serial.println("[TEST] alarm"); break;
      case 'f': bootio.soundOn = true; BEEP(PAT_FINISH); Serial.println("[TEST] melodia"); break;
      case 'm': bootio.soundOn = true; bootio.play(patMinutes, buildMinutePattern(3)); Serial.println("[TEST] 3 minuty"); break;
      case 'r':
        motor.currentmA = S.currentmA;
        motor.setTarget(S.rollRpm);
        Serial.printf("[TEST] silnik START %d rpm, %d mA (UART GPIO21, adres %d)\n", S.rollRpm, S.currentmA, TMC_ADDR);
        break;
      case 's': motor.setTarget(0); Serial.println("[TEST] silnik STOP"); break;
      case 'u': tmcDiag(); break;
      case 'w': tmcDiagOneWire(); break;
      case 'L': gLang = !gLang; fullRedraw = true; Serial.println(gLang ? "[TEST] jezyk EN" : "[TEST] jezyk PL"); break;
      case 'P': {
        captureAll();
        lgfx::LGFX_Sprite cap(&lcd);
        cap.setColorDepth(8);
        if (cap.createSprite(SCR_W, SCR_H)) { G = &cap; drawIntroFrame(0.0f); drawIntroFrame(1.0f); G = &lcd; sendCapture(cap, "00_intro"); cap.deleteSprite(); }
        fullRedraw = true;
        Serial.println("CAPDONE2");
        break;
      }
      case 't':
        if (thermo.valid) Serial.printf("[TEST] temperatura %.2f C\n", thermo.t);
        else Serial.printf("[TEST] brak czujnika DS18B20 (znaleziony: %d)\n", thermo.sensorFound());
        break;
      case 'K': {   // tabela modelu: Paterson Multi Reel 3, 500 ml, pionowo, ROLL, pokoj 21 C, ADOX DEV 3:15 @ 38 C
        const Curve &cv = ctrlCurve(progs[7].s[1].ctrl);
        const float Ta = 21.0f;
        Serial.println("[COOL] woda plukanie czekanie | koreks: po plukaniu  przy chemii | chemia: po wlaniu max koniec | czas zmiana");
        const int waters[] = {40, 45, 50}, rinses[] = {60, 300}, waits[] = {0, 60, 90, 120};
        for (int w : waters)
          for (int rs : rinses)
            for (int wt : waits) {
              float Tw = Ta, Tl = coolPourTemp(w, Tw, 500);
              coolStep(Tl, Tw, Ta, 500, 0, false, rs);
              float Tafter = Tw;
              Tw = coolEmptyStep(Tw, Ta, false, 15 + wt);       // 15 s ociekania + czekanie
              float Tst, Tend, Tmax;
              float tt = coolStepTime(cv, 195, 1.0f, Ta, 500, 0, false, Tw, &Tend, &Tst, &Tmax);
              Serial.printf("[COOL] %2dC  %3ds  %3ds | %5.1f  %5.1f | %5.2f %5.2f %5.2f | %3.0fs %+3.0fs\n",
                            w, rs, wt, Tafter, Tw, Tst, Tmax, Tend, tt, tt - 195);
            }
        break;
      }
      case 'i':
        Serial.printf("[TEST] FW %s, ekran=%d, dotyk 0x%02X SDA=%d SCL=%d, kalibracja=%d, dzwiek=%d, heap=%u\n",
                      FW_VERSION, (int)scr, touchHw.addr, touchHw.sda, touchHw.scl, S.cal.ok, S.sound,
                      (unsigned)ESP.getFreeHeap());
        break;
    }
  }
  if (S.sound == 0) bootio.soundOn = false;
}

void setup() {
  Serial.begin(115200);
  bootio.begin();
  backlightBegin();
  lcd.init();
  storageBegin();
  gLang = constrain((int)prefs.getShort("lang", 0), 0, 1);
  lcd.setRotation(S.flip ? 3 : 1);
  // --- intro 2 s: logo Spool, rozjasnianie, pasek postepu, melodia ---
  backlightSet(S.brightness);
  drawIntroFrame(0.0f);
  uint32_t t0 = millis();
  Serial.println("\n[FILM PROCESSOR] start, FW " FW_VERSION);
  bootio.soundOn = S.sound;
  BEEP(PAT_BOOT);

  motor.currentmA = S.currentmA;
  motor.begin();
  thermo.offset = S.tempOffset10 / 10.0f;
  thermo.begin();

  // --- dotyk: autowykrywanie pinow ---
  bool ok = touchScan(S.touchSda, S.touchScl);

  // dokoncz 2 s intro; przytrzymanie palca >= 1 s = kalibracja dotyku
  bool held = false;
  uint32_t downSince = 0;
  while (millis() - t0 < 2000) {
    uint32_t el = millis() - t0;
    static uint32_t lastFrame = 0;
    if (millis() - lastFrame >= 45) { lastFrame = millis(); drawIntroFrame(el / 2000.0f); }

    bootio.update();
    int16_t rx, ry;
    bool d = (ok && touchReadRaw(rx, ry)) || bootio.isDown();
    // dowolne dotkniecie logo (>= ~60 ms, odfiltrowane zaklocenia) = kalibracja
    if (d) { if (!downSince) downSince = millis(); if (millis() - downSince > 60) held = true; }
    else downSince = 0;
    delay(15);
  }
  drawIntroFrame(1.0f);
  backlightSet(S.brightness);

  if (!ok) {
    Serial.println("[TOUCH] nie znaleziono - dotknij ekranu, ponawiam skanowanie");
    scr = S_NOTOUCH;
    drawFull();
    while (!ok) { delay(700); ok = touchScan(-1, -1); }
  }
  if (touchHw.sda != S.touchSda || touchHw.scl != S.touchScl) {
    S.touchSda = touchHw.sda; S.touchScl = touchHw.scl;
    saveSettings();
  }
  if (held) { drawSplash("Pusc ekran - kalibracja"); uint32_t t1 = millis(); int16_t a, b; while (touchReadRaw(a, b) && millis() - t1 < 5000) delay(20); }

  if (!S.cal.ok || held) calibStart();
  else goStartScreen();
}

void loop() {
  static uint32_t lastTouch = 0, lastDyn = 0, lastHdr = 0;
  uint32_t now = millis();

  bootio.update();
  thermo.update();
  handleSerial();
  if (scr == S_RUN || scr == S_SETUP) runTick();
  agitator.update();
  otaLoop();

  if (now - lastTouch >= 15) { lastTouch = now; handleTouch(); }
  calibTick();
  marqueeTick();
  if (bootio.takePress()) bootPressed();

  if (fullRedraw) { fullRedraw = false; drawFull(); lastDyn = now; }
  if (now - lastDyn >= 250) { lastDyn = now; drawDynamic(); }
  if (now - lastHdr >= 1000) { lastHdr = now; drawHeaderTemp(); }
}
