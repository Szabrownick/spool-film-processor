// =====================================================================
//  bootio.h  -  GPIO9 (przycisk BOOT) jako przycisk START + brzeczyk
//
//  Przycisk BOOT zwiera GPIO9 do GND. Brzeczyk (opcjonalny) jest
//  wlaczany tranzystorem PNP, ktory przewodzi gdy GPIO9 = LOW.
//  - normalnie pin jest WEJSCIEM z podciaganiem -> czytamy przycisk
//  - zeby zapiszczec, na chwile ustawiamy pin jako WYJSCIE LOW
//  Wcisniecie przycisku tez zwiera pin do GND, wiec brzeczyk "klika"
//  przy wcisnieciu - darmowe potwierdzenie dzwiekowe.
//  UWAGA: GPIO9 musi byc HIGH podczas resetu - PNP tego nie zaburza.
// =====================================================================
#pragma once
#include "config.h"

struct Tone { uint16_t f; uint16_t ms; };     // f = 0 -> cisza

// Melodie w zakresie 1,5-3,5 kHz (tam maly glosnik z PC gra najglosniej),
// krotkie przerwy miedzy nutami (staccato) - brzmi czysciej niz legato.
// Nuty: G6=1568 B6=1976 C7=2093 D7=2349 E7=2637 F7=2794 G7=3136 A7=3520
static const Tone PAT_CLICK[]  = {{3400, 15}};
// start kroku: krotkie "pyk-dzyn" o kwinte w gore
static const Tone PAT_OK[]     = {{2093, 60}, {0, 15}, {3136, 110}};
// przejscie do nastepnego kroku ("wlej kolejna chemie"): kwarta w gore
static const Tone PAT_STEP[]   = {{1976, 80}, {0, 25}, {2637, 180}};
// koniec kroku: szybki pasaz w gore z "zakretem" - konczy sie otwarcie,
// jak pytanie "co dalej?" (ok. 0,8 s)
static const Tone PAT_ALARM[]  = {{1568, 70}, {0, 20}, {2093, 70}, {0, 20}, {2637, 140}, {0, 60},
                                  {2349, 70}, {0, 20}, {3136, 260}};
// koniec calego procesu: krotka fanfara (krotko-krotko-krotko-dlugo) i
// zejscie do toniki - brzmi jak domkniecie (ok. 1,5 s)
static const Tone PAT_FINISH[] = {{1568, 85}, {0, 20}, {2093, 85}, {0, 20}, {2637, 85}, {0, 20}, {3136, 240},
                                  {0, 90},
                                  {2794, 85}, {0, 20}, {3520, 85}, {0, 20}, {3136, 120}, {0, 35},
                                  {2637, 120}, {0, 35}, {2093, 420}};
// woda za goraca: szybkie, wysokie pikanie (jak alarm)
static const Tone PAT_HOT[]    = {{4186, 70}, {0, 50}, {4186, 70}, {0, 50}, {4186, 70}, {0, 50}, {4186, 70}, {0, 50}, {4186, 70}};
// woda za zimna: wolne, niskie, opadajace tony
static const Tone PAT_COLD[]   = {{1568, 250}, {0, 120}, {1319, 250}, {0, 120}, {1047, 400}};
// wlaczenie urzadzenia
static const Tone PAT_BOOT[]   = {{2093, 90}, {0, 25}, {2637, 90}, {0, 25}, {3136, 90}, {0, 25}, {4186, 300}};

// N pikniec (pozostale minuty)
static Tone patMinutes[19];
uint8_t buildMinutePattern(int n) {
  n = constrain(n, 1, 10);
  uint8_t k = 0;
  for (int i = 0; i < n; i++) {
    patMinutes[k++] = Tone{3200, 110};
    if (i < n - 1) patMinutes[k++] = Tone{0, 160};
  }
  return k;
}

class BootIO {
public:
  bool soundOn = true;

  void begin() { idle(); }

  void play(const Tone *p, uint8_t n) {
    if (!BUZZER_ENABLED || !soundOn) return;
    pat = p; patLen = n; idx = 0; startNote();
  }

  // wywolywac czesto w petli
  void update() {
    uint32_t now = millis();
    if (pat) {
      if (now - noteStart >= pat[idx].ms) {
        if (++idx >= patLen) { pat = nullptr; idle(); ignoreUntil = now + 30; }
        else startNote();
      }
      return;                               // podczas dzwieku nie czytamy przycisku
    }
    if (now < ignoreUntil) return;
    bool down = digitalRead(PIN_BOOT_BTN) == LOW;
    if (down && !btnDown && now - lastChange > 40) { pressed = true; lastChange = now; }
    if (down != btnDown) lastChange = now;
    btnDown = down;
  }

  // zwraca true jeden raz po wcisnieciu
  bool takePress() { bool p = pressed; pressed = false; return p; }
  bool isDown() const { return btnDown; }

private:
  const Tone *pat = nullptr;
  uint8_t patLen = 0, idx = 0;
  uint32_t noteStart = 0, ignoreUntil = 0, lastChange = 0;
  bool btnDown = false, pressed = false;

  bool pwmAttached = false;

  void idle() {
    if (pwmAttached) { ledcDetach(PIN_BOOT_BTN); pwmAttached = false; }
    pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
  }
  void startNote() {
    noteStart = millis();
    uint16_t f = pat[idx].f;
    if (f == 0) { idle(); return; }
    if (BUZZER_PASSIVE) {
      if (pwmAttached) ledcDetach(PIN_BOOT_BTN);
      pwmAttached = ledcAttach(PIN_BOOT_BTN, f, 8);
      ledcWrite(PIN_BOOT_BTN, 128);
    } else {
      pinMode(PIN_BOOT_BTN, OUTPUT);
      digitalWrite(PIN_BOOT_BTN, LOW);      // PNP przewodzi -> brzeczyk gra
    }
  }
};

BootIO bootio;
#define BEEP(p) bootio.play(p, sizeof(p) / sizeof(p[0]))
