// =====================================================================
//  programs.h  -  programy wywolywania, ustawienia i zapis w pamieci NVS
// =====================================================================
#pragma once
#include <Preferences.h>
#include "compensation.h"
#include "motor.h"
#include "touch.h"

// ---------------------------------------------------------------------
//  Nazwy krokow (krotkie + pelne do ekranu "wlej")
// ---------------------------------------------------------------------
static const char *const STEP_SHORT[] = {
  "HEAT", "PRE", "DEV", "STOP", "FIX", "BLIX", "BLEA", "RINSE", "WASH",
  "STAB", "FD", "CD", "REV", "COND", "WET", "MONO", "HYPO", "FINAL"};
static const char *const STEP_LONG[] = {
  "Podgrzanie", "Kapiel wstepna", "Wywolywacz", "Przerywacz", "Utrwalacz", "Blix",
  "Wybielacz", "Plukanie", "Plukanie", "Stabilizator", "1. wywolywacz", "Wyw. barwny",
  "Odwracacz", "Kondycjoner", "Zwilzacz", "Monokapiel", "Hypo clear", "Plukanie konc."};
static const char *const STEP_LONG_EN[] = {
  "Pre-heat", "Pre-bath", "Developer", "Stop bath", "Fixer", "Blix",
  "Bleach", "Rinse", "Wash", "Stabilizer", "First developer", "Colour dev.",
  "Reversal", "Conditioner", "Wetting agent", "Monobath", "Hypo clear", "Final rinse"};
#define NUM_STEP_NAMES (int16_t)(sizeof(STEP_SHORT) / sizeof(STEP_SHORT[0]))
enum { SN_HEAT, SN_PRE, SN_DEV, SN_STOP, SN_FIX, SN_BLIX, SN_BLEA, SN_RINSE, SN_WASH,
       SN_STAB, SN_FD, SN_CD, SN_REV, SN_COND, SN_WET, SN_MONO, SN_HYPO, SN_FINAL };

enum { CAT_BW = 0, CAT_C41, CAT_E6, CAT_ECN2, CAT_RA4, NUM_CATS };
static const char *const CAT_NAMES[] = {"B&W", "C-41", "E-6", "ECN-2", "RA-4"};

#define MAX_STEPS 12
#define PROG_MAGIC 0xA60F0001UL

struct Step {
  int16_t name;       // indeks w STEP_SHORT
  int16_t timeSec;    // czas nominalny (przy temperaturze nominalnej krzywej)
  int16_t agit;       // AgitMode
  int16_t ctrl;       // CTRL_OFF / CTRL_MON / krzywa
  int16_t monMin10;   // zakres MON *10
  int16_t monMax10;
};

struct Program {
  uint32_t magic;
  char     title[24];
  int16_t  cat;
  int16_t  nSteps;
  Step     s[MAX_STEPS];
};

// ---------------------------------------------------------------------
//  Programy fabryczne
// ---------------------------------------------------------------------
#define NUM_DEFAULT 8
#define NUM_CUSTOM  4
#define NUM_PROGS   (NUM_DEFAULT + NUM_CUSTOM)

static void progInit(Program &p, const char *title, int16_t cat) {
  memset(&p, 0, sizeof(p));
  p.magic = PROG_MAGIC;
  strncpy(p.title, title, sizeof(p.title) - 1);
  p.cat = cat;
}
static void progAdd(Program &p, int16_t name, int16_t sec, int16_t agit, int16_t ctrl,
                    int16_t mn = 0, int16_t mx = 0) {
  if (p.nSteps >= MAX_STEPS) return;
  p.s[p.nSteps++] = Step{name, sec, agit, ctrl, mn, mx};
}
#define CV(i) (int16_t)(CTRL_CURVE0 + (i))   // indeksy w CURVES[]

void defaultProgram(int idx, Program &p) {
  switch (idx) {
    case 0:   // 1.1 B&W ogolny (czas DEV wpisz z tabeli swojego filmu/wywolywacza dla 20 C)
      progInit(p, "Ogolny", CAT_BW);
      progAdd(p, SN_DEV,   480, AG_ROLL, CV(0));
      progAdd(p, SN_STOP,   60, AG_ROLL, CTRL_MON, 180, 240);
      progAdd(p, SN_FIX,   300, AG_ROLL, CTRL_MON, 180, 240);
      progAdd(p, SN_RINSE, 600, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_WET,    60, AG_NONE, CTRL_OFF);
      break;
    case 1:   // 1.2 B&W Cinestill DF96 (26.7 C = 3:00)
      progInit(p, "Cinestill DF96", CAT_BW);
      progAdd(p, SN_MONO,  180, AG_ROLL, CV(1));
      progAdd(p, SN_RINSE, 300, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_WET,    60, AG_NONE, CTRL_OFF);
      break;
    case 2:   // 2.1 C-41 Tetenal
      progInit(p, "Tetenal", CAT_C41);
      progAdd(p, SN_HEAT,  300, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_DEV,   195, AG_ROLL, CV(2));
      progAdd(p, SN_BLIX,  240, AG_ROLL, CV(4));
      progAdd(p, SN_RINSE, 360, AG_ROLL, CTRL_MON, 300, 400);
      progAdd(p, SN_STAB,   60, AG_NONE, CTRL_MON, 200, 400);
      break;
    case 3:   // 2.2 C-41 Cinestill Cs41 (39 C = 3:30, blix 8:00)
      progInit(p, "Cinestill Cs41", CAT_C41);
      progAdd(p, SN_HEAT,  300, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_DEV,   210, AG_ROLL, CV(3));
      progAdd(p, SN_BLIX,  480, AG_ROLL, CV(4));
      progAdd(p, SN_RINSE, 180, AG_ROLL, CTRL_MON, 300, 410);
      progAdd(p, SN_STAB,   60, AG_NONE, CTRL_MON, 200, 410);
      break;
    case 4:   // 3.1 E-6 trzykapielowy (Tetenal / Cinestill D6)
      progInit(p, "3-kapielowy", CAT_E6);
      progAdd(p, SN_HEAT,  300, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_FD,    390, AG_ROLL, CV(5));
      progAdd(p, SN_RINSE, 150, AG_ROLL, CTRL_MON, 330, 390);
      progAdd(p, SN_CD,    360, AG_ROLL, CV(6));
      progAdd(p, SN_RINSE, 150, AG_ROLL, CTRL_MON, 330, 390);
      progAdd(p, SN_BLIX,  360, AG_ROLL, CV(4));
      progAdd(p, SN_RINSE, 240, AG_ROLL, CTRL_MON, 330, 390);
      progAdd(p, SN_STAB,   60, AG_NONE, CTRL_OFF);
      break;
    case 5:   // 4.1 ECN-2 (Kodak 41.1 C)
      progInit(p, "ECN-2", CAT_ECN2);
      progAdd(p, SN_PRE,    10, AG_ROLL, CTRL_MON, 270, 420);
      progAdd(p, SN_RINSE,  60, AG_ROLL, CTRL_MON, 270, 420);
      progAdd(p, SN_DEV,   180, AG_ROLL, CV(7));
      progAdd(p, SN_STOP,   30, AG_ROLL, CTRL_MON, 270, 420);
      progAdd(p, SN_RINSE,  30, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_BLEA,  180, AG_ROLL, CTRL_MON, 270, 420);
      progAdd(p, SN_RINSE,  60, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_FIX,   120, AG_ROLL, CTRL_MON, 270, 420);
      progAdd(p, SN_RINSE, 120, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_STAB,   10, AG_NONE, CTRL_OFF);
      break;
    case 6:   // 5.1 RA-4 papier
      progInit(p, "Arista", CAT_RA4);
      progAdd(p, SN_PRE,    30, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_DEV,    60, AG_ROLL, CV(8));
      progAdd(p, SN_STOP,   30, AG_ROLL, CTRL_OFF);
      progAdd(p, SN_BLIX,   60, AG_ROLL, CTRL_MON, 200, 380);
      progAdd(p, SN_RINSE, 120, AG_ROLL, CTRL_OFF);
      break;
    case 7:   // 2.3 C-41 ADOX C-TEC 41 1L (karta producenta 9/2026, 38 C)
      progInit(p, "ADOX C-TEC 41", CAT_C41);
      progAdd(p, SN_HEAT,  300, AG_ROLL, CTRL_OFF);          // podgrzanie koreksu 5:00
      progAdd(p, SN_DEV,   195, AG_ROLL, CV(9));             // 3:15 @38 (8:00 @30)
      progAdd(p, SN_RINSE,  30, AG_ROLL, CTRL_MON, 250, 430); // plukanie cieple 0:30
      progAdd(p, SN_BLIX,  240, AG_ROLL, CV(4));             // 4:00 @38 (6:00 @30)
      progAdd(p, SN_RINSE, 360, AG_ROLL, CTRL_MON, 300, 400); // plukanie 6:00, 30-40 C
      progAdd(p, SN_STAB,   60, AG_NONE, CTRL_MON, 200, 400); // stabilizator 1:00, 20-40 C
      break;
    default: {  // programy wlasne - poczatkowo kopia B&W
      defaultProgram(0, p);
      char t[24];
      snprintf(t, sizeof(t), "Wlasny %d", idx - NUM_DEFAULT + 1);
      strncpy(p.title, t, sizeof(p.title) - 1);
    }
  }
}

Program progs[NUM_PROGS];

bool isCustom(int idx) { return idx >= NUM_DEFAULT; }

// numer programu: "2.1" = kategoria 2, pierwszy program; wlasne "W1"
void progNumber(int idx, char *out, size_t n) {
  if (isCustom(idx)) { snprintf(out, n, "W%d", idx - NUM_DEFAULT + 1); return; }
  int cat = progs[idx].cat, k = 1;
  for (int i = 0; i < idx; i++) if (!isCustom(i) && progs[i].cat == cat) k++;
  snprintf(out, n, "%d.%d", cat + 1, k);
}

// ---------------------------------------------------------------------
//  Ustawienia
// ---------------------------------------------------------------------
#define SET_MAGIC 0x5E770002UL

struct Settings {
  uint32_t magic;
  int16_t sound, brightness, flip, tempF;
  int16_t bwRed, bwRedPct;
  int16_t currentmA, tempOffset10;
  int16_t rollRpm, rollT1, rollT2;
  int16_t stickRpm, stickT1, stickT2;
  int16_t standRpm, standT1, standT2, standT3;
  TouchCal cal;
  int8_t touchSda, touchScl;
  int16_t noMinuteBeep;     // 0 = pikanie co minute WLACZONE (miesci sie w dawnym wyrownaniu
                            //     struktury, wiec zapisane ustawienia i kalibracja zostaja)
};

static_assert(sizeof(Settings) == 64, "zmiana rozmiaru Settings skasuje zapisane ustawienia");
Settings S;

void settingsDefault(Settings &s, bool keepTouch) {
  TouchCal cal = s.cal;
  int8_t sda = s.touchSda, scl = s.touchScl;
  memset(&s, 0, sizeof(s));
  s.magic = SET_MAGIC;
  s.sound = 1; s.brightness = 100; s.flip = 0; s.tempF = 0;
  s.bwRed = 0; s.bwRedPct = 15;
  s.currentmA = 700; s.tempOffset10 = 0;
  s.rollRpm = 60;  s.rollT1 = 15;  s.rollT2 = 14;      // wartosci domyslne
  s.stickRpm = 60; s.stickT1 = 60; s.stickT2 = 10;
  s.standRpm = 60; s.standT1 = 30; s.standT2 = 60; s.standT3 = 30;
  if (keepTouch) { s.cal = cal; s.touchSda = sda; s.touchScl = scl; }
  else { s.cal = TouchCal(); s.touchSda = -1; s.touchScl = -1; }
}

// Dodatkowe ustawienia ROLL (start intensywny, burst) - osobne klucze w pamieci,
// bo blok Settings ma stale 64 bajty (zmiana rozmiaru skasowalaby kalibracje)
int16_t rollStartSec = 30;    // ADOX C-TEC 41: pierwsze 30 s mieszac intensywnie
int16_t rollBurstSec = 30;    // co 30 s seria zmian kierunku
int16_t rollBurstN = 4;       // 4 zmiany kierunku w serii
AgitParams agitParams() {
  AgitParams p;
  p.rollRpm = S.rollRpm;   p.rollT1 = S.rollT1;   p.rollT2 = S.rollT2;
  p.rollStartSec = rollStartSec; p.rollBurstSec = rollBurstSec; p.rollBurstN = rollBurstN;
  p.stickRpm = S.stickRpm; p.stickT1 = S.stickT1; p.stickT2 = S.stickT2;
  p.standRpm = S.standRpm; p.standT1min = S.standT1; p.standT2 = S.standT2; p.standT3 = S.standT3;
  return p;
}

// ---------------------------------------------------------------------
//  Zapis / odczyt NVS
// ---------------------------------------------------------------------
Preferences prefs;

// Stale numery kluczy w pamieci: programy dopisane pozniej dostaja nowe numery,
// zeby zapisane wczesniej programy (fabryczne 0-6 i wlasne "p7".."p10") zostaly na miejscu.
static int progSlot(int idx) {
  if (idx < 7) return idx;            // fabryczne z wersji 1.0
  if (idx == 7) return 20;            // ADOX C-TEC 41 (dodany pozniej)
  return idx - 1;                     // wlasne: 8..11 -> 7..10
}
static void progKey(int idx, char *k) { snprintf(k, 6, "p%d", progSlot(idx)); }

// ---------------------------------------------------------------------
//  Plukanie zmianami wody: dla kazdego kroku liczba zmian (0 = ciagle).
//  Czas kroku to czas laczny; jedna zmiana trwa czas / liczba zmian.
//  Zapisywane osobno ("r<slot>"), zeby nie zmieniac formatu programow.
// ---------------------------------------------------------------------
int16_t rinseChg[NUM_PROGS][MAX_STEPS];

static void rinseKey(int idx, char *k) { snprintf(k, 6, "r%d", progSlot(idx)); }
void saveRinse(int idx) { char k[6]; rinseKey(idx, k); prefs.putBytes(k, rinseChg[idx], sizeof(rinseChg[idx])); }

// domyslnie: dlugie plukania (>= 2 min) robimy zmianami wody, ok. 1 zmiana na minute
void rinseDefault(int idx) {
  const Program &p = progs[idx];
  for (int i = 0; i < MAX_STEPS; i++) {
    int16_t n = 0;
    if (i < p.nSteps && (p.s[i].name == SN_RINSE || p.s[i].name == SN_WASH) && p.s[i].timeSec >= 120)
      n = constrain(p.s[i].timeSec / 60, 2, 8);
    rinseChg[idx][i] = n;
  }
}

void saveProgram(int idx) {
  char k[6]; progKey(idx, k);
  prefs.putBytes(k, &progs[idx], sizeof(Program));
  saveRinse(idx);
}

// ---------------------------------------------------------------------
//  Stan chemii dla programu: zestaw (tabela zuzycia), zuzyte rolki,
//  ile rolek ostatnio bylo w koreksie. Zapisywane osobno ("c<slot>").
// ---------------------------------------------------------------------
struct ChemState { int16_t kit; int16_t used; int16_t tank; int16_t magic; };
#define CHEM_MAGIC 0x0C41
ChemState chem[NUM_PROGS];

int16_t defaultKit(int idx) {
  switch (idx) {
    case 2: return KIT_TETENAL_1L;
    case 3: return KIT_CS41;
    case 7: return KIT_ADOX_1L;
    default: return KIT_NONE;
  }
}
static void chemKey(int idx, char *k) { snprintf(k, 6, "c%d", progSlot(idx)); }
void saveChem(int idx) { char k[6]; chemKey(idx, k); prefs.putBytes(k, &chem[idx], sizeof(ChemState)); }
void chemDefault(int idx) { chem[idx] = ChemState{defaultKit(idx), 0, 1, CHEM_MAGIC}; }

// ktora tabela zuzycia dotyczy kroku
KitStepType kitStepType(const Step &s) {
  switch (s.name) {
    case SN_DEV: case SN_FD: case SN_CD: case SN_MONO: return KS_DEV;
    case SN_BLIX: case SN_BLEA: case SN_FIX: return KS_BX;
    default: return KS_NONE;
  }
}
void saveSettings() {
  prefs.putBytes("set", &S, sizeof(Settings));
  prefs.putShort("rS", rollStartSec); prefs.putShort("rB", rollBurstSec); prefs.putShort("rN", rollBurstN);
}

void saveChem(int idx);
void chemDefault(int idx);
void resetProgram(int idx) { defaultProgram(idx, progs[idx]); rinseDefault(idx); saveProgram(idx); chemDefault(idx); saveChem(idx); }

void storageBegin() {
  prefs.begin("filmproc", false);
  if (prefs.getBytesLength("set") == sizeof(Settings)) prefs.getBytes("set", &S, sizeof(Settings));
  if (S.magic != SET_MAGIC) { S.touchSda = -1; S.touchScl = -1; settingsDefault(S, false); }
  rollStartSec = constrain((int)prefs.getShort("rS", 30), 0, 120);
  rollBurstSec = constrain((int)prefs.getShort("rB", 30), 0, 120);
  rollBurstN = constrain((int)prefs.getShort("rN", 4), 1, 8);
  for (int i = 0; i < NUM_PROGS; i++) {
    char k[6]; progKey(i, k);
    bool ok = prefs.getBytesLength(k) == sizeof(Program) && prefs.getBytes(k, &progs[i], sizeof(Program)) &&
              progs[i].magic == PROG_MAGIC && progs[i].nSteps <= MAX_STEPS;
    if (!ok) defaultProgram(i, progs[i]);
    char c[6]; chemKey(i, c);
    bool okc = prefs.getBytesLength(c) == sizeof(ChemState) && prefs.getBytes(c, &chem[i], sizeof(ChemState)) &&
               chem[i].magic == CHEM_MAGIC && chem[i].kit >= 0 && chem[i].kit < NUM_KITS;
    if (!okc) chemDefault(i);
    char r[6]; rinseKey(i, r);
    bool okr = prefs.getBytesLength(r) == sizeof(rinseChg[i]) && prefs.getBytes(r, rinseChg[i], sizeof(rinseChg[i]));
    if (!okr) rinseDefault(i);
    for (int j = 0; j < MAX_STEPS; j++) rinseChg[i][j] = constrain(rinseChg[i][j], 0, 12);
  }
}

// czas nominalny kroku po ew. automatycznej redukcji B&W (tylko DEV
// w programach B&W, z wyjatkiem DF96)
float stepBaseSec(const Program &p, const Step &s) {
  float t = s.timeSec;
  if (S.bwRed && p.cat == CAT_BW && s.name == SN_DEV && s.ctrl != CV(1))
    t *= (100 - S.bwRedPct) / 100.0f;
  return t;
}
// czas kroku przy temperaturze nominalnej, z doliczeniem zuzycia chemii programu
float stepNominalSec(int idx, const Step &s) {
  return stepBaseSec(progs[idx], s) * kitFactor(chem[idx].kit, kitStepType(s), chem[idx].used, NAN);
}

// kompensacja wylaczona dla STICK i STAND - czujnik nie dotyka chemii
bool stepAllowsComp(const Step &s) { return s.agit == AG_ROLL || s.agit == AG_NONE; }
