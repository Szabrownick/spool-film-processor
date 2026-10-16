// =====================================================================
//  motor.h  -  TMC2209 sterowany JEDNYM pinem (UART, tylko zapis)
//
//  Zamiast impulsow STEP/DIR uzywamy rejestru VACTUAL: sterownik sam
//  generuje kroki z zadana predkoscia (znak = kierunek). Dzieki temu:
//    - wystarcza 1 GPIO (TX -> 1k -> PDN_UART),
//    - ruch jest idealnie plynny niezaleznie od obciazenia procesora,
//    - prad silnika i wlaczanie/wylaczanie ustawiamy programowo.
// =====================================================================
#pragma once
#include "config.h"

// --- rejestry TMC2209 ---
#define TMC_REG_GCONF      0x00
#define TMC_REG_IHOLD_IRUN 0x10
#define TMC_REG_TPOWERDOWN 0x11
#define TMC_REG_VACTUAL    0x22
#define TMC_REG_CHOPCONF   0x6C

class TMC2209Uart {
public:
  void begin() {
    // RX nieuzywany (-1): komunikacja tylko w strone sterownika
    Serial1.begin(TMC_BAUD, SERIAL_8N1, -1, PIN_TMC_UART);
  }

  void write(uint8_t reg, uint32_t val) {
    uint8_t d[8] = {0x05, TMC_ADDR, (uint8_t)(reg | 0x80),
                    (uint8_t)(val >> 24), (uint8_t)(val >> 16),
                    (uint8_t)(val >> 8), (uint8_t)val, 0};
    d[7] = crc(d, 7);
    Serial1.write(d, 8);
  }

  // Konfiguracja: stealthChop (cicho), 256 mikrokrokow, prad z UART
  void configure(uint16_t currentmA, bool enabled) {
    // GCONF: pdn_disable(6)=1, mstep_reg_select(7)=1, multistep_filt(8)=1
    //        I_scale_analog(0)=0 -> wewnetrzne zrodlo odniesienia (potencjometr Vref ignorowany)
    uint32_t gconf = (1UL << 6) | (1UL << 7) | (1UL << 8);
    if (MOTOR_INVERT) gconf |= (1UL << 3);   // shaft
    write(TMC_REG_GCONF, gconf);

    // Prad: Irms = (CS+1)/32 * Vfs/(Rsense+0.02) / sqrt(2)
    float I = currentmA / 1000.0f;
    bool vsense = true;
    float imax = 0.180f / (TMC_RSENSE + 0.02f) / 1.41421f;    // ~0.98A dla 0.11R
    if (I > imax * 0.95f) { vsense = false; imax = 0.325f / (TMC_RSENSE + 0.02f) / 1.41421f; }
    int cs = (int)roundf(I / imax * 32.0f) - 1;
    cs = constrain(cs, 0, 31);
    int ihold = cs / 3;
    write(TMC_REG_IHOLD_IRUN, (uint32_t)ihold | ((uint32_t)cs << 8) | (6UL << 16));
    write(TMC_REG_TPOWERDOWN, 20);

    // CHOPCONF: TOFF=3 (0 = stopien mocy wylaczony), HSTRT=4, HEND=1, TBL=2,
    //           vsense, MRES=0 (256 mikrokrokow), intpol=1
    uint32_t chop = (enabled ? 3UL : 0UL) | (4UL << 4) | (1UL << 7) | (2UL << 15) |
                    ((vsense ? 1UL : 0UL) << 17) | (0UL << 24) | (1UL << 28);
    write(TMC_REG_CHOPCONF, chop);
  }

  // Predkosc w obr/min koreksu (ze znakiem). 0 = stop.
  void setRpm(float rpm) {
    // VACTUAL [mikrokroki / t], t = 2^24 / fCLK(12MHz) -> 1 jednostka = 0.715 mikrokroku/s
    float usteps = rpm * GEAR_RATIO / 60.0f * MOTOR_STEPS * 256.0f;
    int32_t v = (int32_t)lroundf(usteps / 0.7152557f);
    write(TMC_REG_VACTUAL, (uint32_t)v & 0xFFFFFF);
  }

public:   // crc uzywane tez przez diagnostyke
  static uint8_t crc(const uint8_t *d, uint8_t len) {
    uint8_t c = 0;
    for (uint8_t i = 0; i < len; i++) {
      uint8_t b = d[i];
      for (uint8_t j = 0; j < 8; j++) {
        if ((c >> 7) ^ (b & 0x01)) c = (c << 1) ^ 0x07;
        else c = c << 1;
        b >>= 1;
      }
    }
    return c;
  }
};

TMC2209Uart tmc;

// =====================================================================
//  Motor - rampy predkosci i zmiana kierunku przez zero
// =====================================================================
class Motor {
public:
  uint16_t currentmA = 700;

  void begin() {
    tmc.begin();
    delay(5);
    tmc.configure(currentmA, false);
    tmc.setRpm(0);
  }

  // docelowa predkosc ze znakiem (+ = CW, - = CCW), 0 = zatrzymanie
  void setTarget(float rpm) { target = rpm; }
  float speed() const { return cur; }
  bool isEnabled() const { return enabled; }

  void update() {
    uint32_t now = millis();
    uint32_t dt = now - lastMs;
    if (dt < 20) return;
    lastMs = now;

    if (target != 0 && !enabled) {          // wlacz stopien mocy
      enabled = true;
      tmc.configure(currentmA, true);
      lastCfg = now;
    }
    float step = MOTOR_ACCEL_RPM_S * dt / 1000.0f;
    float prev = cur;
    if (cur < target) cur = min(cur + step, target);
    else if (cur > target) cur = max(cur - step, target);
    if (cur != prev) tmc.setRpm(cur);

    if (enabled) {
      // okresowo ponow konfiguracje (np. gdy zasilanie 12V pojawilo sie pozniej)
      if (now - lastCfg > 3000) { tmc.configure(currentmA, true); tmc.setRpm(cur); lastCfg = now; }
      if (target == 0 && cur == 0) {
        if (stopSince == 0) stopSince = now;
        if (now - stopSince > 400) {         // po zatrzymaniu: wylacz (oszczednosc baterii)
          enabled = false;
          tmc.configure(currentmA, false);
          stopSince = 0;
        }
      } else stopSince = 0;
    }
  }

  void reconfigure() { tmc.configure(currentmA, enabled); }

private:
  float target = 0, cur = 0;
  bool enabled = false;
  uint32_t lastMs = 0, lastCfg = 0, stopSince = 0;
};

Motor motor;

// =====================================================================
//  Agitator - tryby agitacji ROLL / STICK / STAND
//   ROLL : obroty ciagle, T1 s w prawo, T2 s w lewo, powtarzane
//   STICK: T1 s obrotow ciaglych, potem T2 s obrotow co minute
//   STAND: T2 s na poczatku, potem co T1 minut T3 s obrotow
//   BRAK : silnik stoi (np. stabilizator)
// =====================================================================
enum AgitMode : int16_t { AG_ROLL = 0, AG_STICK = 1, AG_STAND = 2, AG_NONE = 3 };
static const char *const AGIT_NAMES[] = {"ROLL", "STICK", "STAND", "BRAK"};

struct AgitParams {           // wypelniane z ustawien
  int16_t rollRpm, rollT1, rollT2;
  int16_t stickRpm, stickT1, stickT2;
  int16_t standRpm, standT1min, standT2, standT3;
};

class Agitator {
public:
  void start(AgitMode m, const AgitParams &p) { mode = m; par = p; t0 = millis(); pausedMs = 0; running = true; paused = false; }
  void stop() { running = false; motor.setTarget(0); }
  void pause(bool on) {
    if (on == paused || !running) return;
    paused = on;
    if (on) pauseStart = millis(); else pausedMs += millis() - pauseStart;
  }
  bool isRunning() const { return running; }

  void update() {
    if (!running) { motor.update(); return; }              // silnikiem steruje ktos inny (test)
    if (paused) { motor.setTarget(0); motor.update(); return; }
    float t = (millis() - t0 - pausedMs) / 1000.0f;   // sekundy od startu kroku
    float rpm = 0;
    switch (mode) {
      case AG_ROLL: {
        float cyc = max(1, (int)par.rollT1 + (int)par.rollT2);
        float ph = fmodf(t, cyc);
        rpm = (ph < par.rollT1) ? par.rollRpm : -par.rollRpm;
        break;
      }
      case AG_STICK: {
        if (t < par.stickT1) rpm = par.stickRpm;
        else {
          float s = t - par.stickT1;
          int minute = (int)(s / 60.0f);
          float inMin = s - minute * 60.0f;
          rpm = (inMin < par.stickT2) ? ((minute & 1) ? -par.stickRpm : par.stickRpm) : 0;
        }
        break;
      }
      case AG_STAND: {
        if (t < par.standT2) rpm = par.standRpm;
        else {
          float s = t - par.standT2;
          float cyc = max(1, (int)par.standT1min) * 60.0f;
          int n = (int)(s / cyc);
          float inCyc = s - n * cyc;
          // obroty T3 s na koncu kazdego okresu postoju
          rpm = (inCyc >= cyc - par.standT3) ? ((n & 1) ? -par.standRpm : par.standRpm) : 0;
        }
        break;
      }
      default: rpm = 0;
    }
    motor.setTarget(rpm);
    motor.update();
  }

private:
  AgitMode mode = AG_NONE;
  AgitParams par{};
  uint32_t t0 = 0, pausedMs = 0, pauseStart = 0;
  bool running = false, paused = false;
};

Agitator agitator;
