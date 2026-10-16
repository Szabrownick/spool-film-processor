// =====================================================================
//  compensation.h  -  KOMPENSACJA CZASU OD TEMPERATURY
//
//  1) MODEL
//     Szybkosc wywolywania rosnie z temperatura w przyblizeniu
//     wykladniczo (prawo Arrheniusa). Dla kazdej chemii mamy krzywa
//     F(T) = czas_przy_T / czas_przy_T_nom   (F(T_nom) = 1.0)
//     zapisana jako kilka punktow z tabel producenta. Miedzy punktami
//     interpolujemy LINIOWO W LOGARYTMIE:
//         ln F(T) = ln F1 + (ln F2 - ln F1) * (T - T1) / (T2 - T1)
//     co daje dokladnie przebieg wykladniczy miedzy punktami.
//     Poza tabela przedluzamy ostatni odcinek (maks. 5 st. C) i
//     ograniczamy F do zakresu 0.25 .. 4.0.
//
//  2) CALKOWANIE W CZASIE RZECZYWISTYM ("przez caly proces")
//     Temperatura w koreksie zmienia sie w trakcie kroku (chemia stygnie).
//     Dlatego nie liczymy czasu raz na poczatku, tylko co ~100 ms
//     dodajemy "postep wywolania":
//         postep += dt / (t_nom * F(T_teraz))
//     Krok konczy sie, gdy postep >= 1.0.
//     Pozostaly czas wyswietlany = (1 - postep) * t_nom * F(T_teraz)
//     Dodany czas ("+0:42")      = (uplynelo + pozostalo) - t_nom
//
//  3) PRZYKLAD  C-41 wywolywacz, t_nom = 3:15 przy 38.0 C
//     chemia ma 36.0 C:  F = 1.29  -> czas ~4:12 (+0:57)
//     jesli w polowie kroku spadnie do 35.0 C, druga polowa liczy sie
//     juz z F(35) = 1.45, wiec calosc wyjdzie ok. 4:27
//     (2:06 przy 36 C + 2:21 przy 35 C).
//
//  !!! Krzywe ponizej sa PRZYBLIZONE na podstawie ogolnodostepnych
//  tabel czas/temperatura (Ilford, Tetenal, Cinestill, Kodak).
//  Sprawdz z karta katalogowa SWOJEJ chemii i popraw punkty, jesli
//  sie roznia: F = (czas w tabeli przy T) / (czas przy T_nom).
// =====================================================================
#pragma once
#include <math.h>

struct CurvePt { int16_t t10; uint16_t f1000; };   // temperatura*10, wspolczynnik*1000

struct Curve {
  const char *name;       // pelna nazwa
  const char *tag;        // krotka nazwa na ekran przebiegu
  int16_t nom10;          // temperatura nominalna *10 (do niej odnosi sie czas w programie)
  int16_t lo10, hi10;     // zakres "w normie" (poza nim tlo termometru czerwone)
  const CurvePt *pts;
  uint8_t n;
};

// ---- B&W ogolny: wykres Ilford (ok. -9.5% czasu na kazdy +1 C) ----
static const CurvePt C_BW[] = {
  {140, 1770}, {160, 1460}, {180, 1210}, {190, 1100}, {200, 1000},
  {210, 910}, {220, 830}, {230, 750}, {240, 680}, {260, 560}};
// ---- B&W Cinestill DF96 (monokapiel): 21.1C 6:00 / 23.9C 4:00 / 26.7C 3:00 ----
static const CurvePt C_DF96[] = {
  {211, 2000}, {239, 1330}, {267, 1000}, {294, 830}};
// ---- C-41 wywolywacz Tetenal / Bellini / Fuji (38.0C = 3:15) ----
static const CurvePt C_C41T[] = {
  {250, 3850}, {300, 2460}, {320, 2000}, {340, 1620}, {360, 1290},
  {370, 1140}, {380, 1000}, {390, 920}, {400, 850}};
// ---- C-41 wywolywacz Cinestill Cs41 / Arista / Unicolor (39.0C = 3:30) ----
static const CurvePt C_C41C[] = {
  {240, 2900}, {267, 2290}, {300, 1860}, {330, 1500}, {360, 1210},
  {390, 1000}, {410, 870}};
// ---- C-41 / E-6 BLIX (mniej wrazliwy, 38.0C) ----
static const CurvePt C_BLIX[] = {
  {240, 2200}, {300, 1500}, {350, 1150}, {380, 1000}, {400, 920}};
// ---- E-6 pierwszy wywolywacz (bardzo wrazliwy, 38.0C) ----
static const CurvePt C_E6FD[] = {
  {340, 1460}, {360, 1210}, {370, 1100}, {380, 1000}, {390, 910}, {400, 830}};
// ---- E-6 wywolywacz barwny (38.0C) ----
static const CurvePt C_E6CD[] = {
  {340, 1250}, {360, 1120}, {380, 1000}, {400, 900}};
// ---- ECN-2 wywolywacz (Kodak SM-2 41.1C = 3:00) ----
static const CurvePt C_ECN2[] = {
  {300, 2900}, {340, 2000}, {360, 1580}, {389, 1170}, {411, 1000}, {430, 870}};
// ---- RA-4 wywolywacz (35.0C) ----
static const CurvePt C_RA4[] = {
  {200, 4000}, {250, 2600}, {300, 1600}, {330, 1250}, {350, 1000}, {380, 800}};

// ---- C-41 ADOX C-TEC 41 (karta producenta 9/2026: 38C = 3:15, 30C = 8:00,
//      zakres pracy 24..38C; ponizej 30C przedluzony ten sam spadek logarytmiczny) ----
static const CurvePt C_ADOX[] = {
  {240, 4830}, {300, 2462}, {380, 1000}, {390, 920}};

#define N_(a) (uint8_t)(sizeof(a) / sizeof(a[0]))
static const Curve CURVES[] = {
  {"B&W ogolny (Ilford)", "BW",    200, 180, 240, C_BW,   N_(C_BW)},
  {"B&W Cinestill DF96",  "DF96",  267, 210, 300, C_DF96, N_(C_DF96)},
  {"C-41 Tetenal/Bellini","C41T",  380, 300, 400, C_C41T, N_(C_C41T)},
  {"C-41 Cinestill/Arista","C41C", 390, 240, 410, C_C41C, N_(C_C41C)},
  {"BLIX C-41/E-6",       "BLIX",  380, 300, 400, C_BLIX, N_(C_BLIX)},
  {"E-6 1. wywolywacz",   "E6FD",  380, 360, 400, C_E6FD, N_(C_E6FD)},
  {"E-6 wyw. barwny",     "E6CD",  380, 350, 400, C_E6CD, N_(C_E6CD)},
  {"ECN-2 wywolywacz",    "ECN2",  411, 300, 430, C_ECN2, N_(C_ECN2)},
  {"RA-4 wywolywacz",     "RA4",   350, 250, 380, C_RA4,  N_(C_RA4)},
  {"C-41 ADOX C-TEC 41",  "ADOX",  380, 240, 390, C_ADOX, N_(C_ADOX)},
};
#define NUM_CURVES N_(CURVES)

// Kontrola czasu w kroku:  0 = WYL, 1 = MON (tylko pilnowanie zakresu), 2.. = krzywa (2 + indeks)
#define CTRL_OFF 0
#define CTRL_MON 1
#define CTRL_CURVE0 2
#define NUM_CTRL (2 + NUM_CURVES)

inline bool ctrlIsCurve(int16_t c) { return c >= CTRL_CURVE0 && c < NUM_CTRL; }
inline const Curve &ctrlCurve(int16_t c) { return CURVES[c - CTRL_CURVE0]; }

static const char *ctrlName(int16_t c) {
  if (c == CTRL_OFF) return "WYL";
  if (c == CTRL_MON) return "MON";
  if (ctrlIsCurve(c)) return ctrlCurve(c).tag;
  return "?";
}

// ---------------------------------------------------------------------
//  F(T) - wspolczynnik czasu dla krzywej przy temperaturze T [C]
// ---------------------------------------------------------------------
float curveFactor(const Curve &cv, float T) {
  const CurvePt *p = cv.pts;
  uint8_t n = cv.n;
  float t = T * 10.0f;
  // ogranicz ekstrapolacje do 5 st. C poza tabela
  t = constrain(t, (float)p[0].t10 - 50.0f, (float)p[n - 1].t10 + 50.0f);
  uint8_t i = 0;                                   // wybierz odcinek [i, i+1]
  while (i < n - 2 && t > p[i + 1].t10) i++;
  float t1 = p[i].t10, t2 = p[i + 1].t10;
  float l1 = logf(p[i].f1000 / 1000.0f), l2 = logf(p[i + 1].f1000 / 1000.0f);
  float l = l1 + (l2 - l1) * (t - t1) / (t2 - t1);
  return constrain(expf(l), 0.25f, 4.0f);
}

// =====================================================================
//  ZUZYCIE CHEMII - doliczanie czasu za kolejne rolki
//
//  Tabele producenta podaja czasy dla grup rolek (np. 1-4, 5-8, ...)
//  przy dwoch temperaturach (38C i 30C). Zapisujemy je jako mnozniki
//  wzgledem pierwszej grupy:  M = czas_grupy / czas_swiezej_chemii.
//  Dla temperatury posredniej mnoznik interpolujemy liniowo miedzy
//  tabela "ciepla" (38C) i "zimna" (30C), bo przy nizszej temperaturze
//  zuzycie wydluza czas wyraznie bardziej (ADOX: +15 s przy 38C,
//  ale +60 s przy 30C na grupe).
//
//  Czas kroku = czas_nominalny * F(T) [temperatura] * M(rolki, T) [zuzycie]
//
//  Przyklad ADOX 1L, 30C, juz zuzyte 6 rolek (grupa 5-8):
//    wywolywacz: 3:15 * 2.46 * 1.125 = 9:00  (zgodnie z tabela)
//    blix:       4:00 * 1.50 * 1.333 = 8:00  (zgodnie z tabela)
// =====================================================================
struct ChemKit {
  const char *name;
  uint8_t rollsPerStage;      // ile rolek w jednej grupie tabeli (0 = model procentowy)
  uint8_t maxRolls;           // wydajnosc zestawu wg producenta
  int16_t hotT10, coldT10;    // temperatury dwoch tabel *10
  uint16_t devHot[4], devCold[4];   // mnozniki *1000 dla wywolywacza
  uint16_t bxHot[4], bxCold[4];     // mnozniki *1000 dla blixu / wybielacza / utrwalacza
  uint16_t devPctPerRoll;     // model procentowy: +x/10 % czasu wywolywacza na rolke
};

// Tabele ADOX i Tetenal sa identyczne (instrukcje producentow, 38C i 30C):
//   wyw. 38C: 3:15 / 3:30 / 3:45 / 4:00     blix 38C: 4:00 / 6:00 / 10:00 / 15:00
//   wyw. 30C: 8:00 / 9:00 / 10:00 / 11:00   blix 30C: 6:00 / 8:00 / 12:00 / 20:00
#define ADOX_DEV_HOT  {1000, 1077, 1154, 1231}
#define ADOX_DEV_COLD {1000, 1125, 1250, 1375}
#define ADOX_BX_HOT   {1000, 1500, 2500, 3750}
#define ADOX_BX_COLD  {1000, 1333, 2000, 3333}
#define FLAT4         {1000, 1000, 1000, 1000}

static const ChemKit KITS[] = {
  // 0: brak doliczania (chemia jednorazowa, np. E-6 1. wywolywacz, wiekszosc B&W)
  {"Brak doliczania", 0, 0, 380, 300, FLAT4, FLAT4, FLAT4, FLAT4, 0},
  // 1-2: ADOX C-TEC 41 (1 L: grupy po 4 rolki, max 16; 0,5 L: po 2 rolki, max 8)
  {"ADOX C-TEC 41 1L",   4, 16, 380, 300, ADOX_DEV_HOT, ADOX_DEV_COLD, ADOX_BX_HOT, ADOX_BX_COLD, 0},
  {"ADOX C-TEC 41 0,5L", 2,  8, 380, 300, ADOX_DEV_HOT, ADOX_DEV_COLD, ADOX_BX_HOT, ADOX_BX_COLD, 0},
  // 3-4: Tetenal Colortec C-41 - ta sama tabela co ADOX
  {"Tetenal C-41 1L",    4, 16, 380, 300, ADOX_DEV_HOT, ADOX_DEV_COLD, ADOX_BX_HOT, ADOX_BX_COLD, 0},
  {"Tetenal C-41 0,5L",  2,  8, 380, 300, ADOX_DEV_HOT, ADOX_DEV_COLD, ADOX_BX_HOT, ADOX_BX_COLD, 0},
  // 5: Bellini C-41 - +15 s wywolywacza co 3 rolki; wybielacz i utrwalacz osobno, bez zmian
  {"Bellini C-41 1L",    3, 12, 380, 300, ADOX_DEV_HOT, ADOX_DEV_HOT, FLAT4, FLAT4, 0},
  // 6: Cinestill Cs41 - +2% czasu wywolywacza na kazda rolke
  {"Cinestill Cs41",     0, 24, 380, 300, FLAT4, FLAT4, FLAT4, FLAT4, 20},
  // 7: ogolny - gdy producent nie podaje tabeli
  {"Ogolny +2%/rolke",   0, 20, 380, 300, FLAT4, FLAT4, FLAT4, FLAT4, 20},
};
#define NUM_KITS (int16_t)(sizeof(KITS) / sizeof(KITS[0]))
#define KIT_NONE 0
#define KIT_ADOX_1L 1
#define KIT_TETENAL_1L 3
#define KIT_CS41 6

enum KitStepType : uint8_t { KS_NONE, KS_DEV, KS_BX };

// mnoznik czasu za zuzycie chemii; T = NaN -> tabela "ciepla" (nominalna)
float kitFactor(int16_t kitId, KitStepType st, int rollsUsed, float T) {
  if (kitId <= KIT_NONE || kitId >= NUM_KITS || st == KS_NONE || rollsUsed <= 0) return 1.0f;
  const ChemKit &k = KITS[kitId];
  if (k.rollsPerStage == 0) {                       // model procentowy
    return st == KS_DEV ? 1.0f + k.devPctPerRoll / 1000.0f * rollsUsed : 1.0f;
  }
  // Bez limitu rolek: po ostatniej grupie tabeli czas rosnie dalej o ten sam
  // przyrost co miedzy dwiema ostatnimi grupami (ADOX: "overclocking" zestawu).
  int stage = rollsUsed / k.rollsPerStage;
  const uint16_t *hot = st == KS_DEV ? k.devHot : k.bxHot;
  const uint16_t *cold = st == KS_DEV ? k.devCold : k.bxCold;
  auto at = [&](const uint16_t *t) -> float {
    if (stage <= 3) return t[stage] / 1000.0f;
    return (t[3] + (stage - 3) * (int)(t[3] - t[2])) / 1000.0f;
  };
  float fh = at(hot), fc = at(cold);
  if (isnan(T)) return fh;
  float w = (T * 10.0f - k.coldT10) / (float)(k.hotT10 - k.coldT10);   // 0 = zimno, 1 = cieplo
  w = constrain(w, 0.0f, 1.0f);
  return fc + (fh - fc) * w;
}

// ---------------------------------------------------------------------
//  Licznik czasu kroku z kompensacja (calkowanie postepu)
// ---------------------------------------------------------------------
struct StepTimer {
  float baseSec = 0;      // czas nominalny (po ew. redukcji B&W)
  float progress = 0;     // 0..1
  float elapsed = 0;      // sekundy rzeczywiste
  float factor = 1;       // ostatni wspolczynnik F
  bool  compensate = false;
  int16_t ctrl = CTRL_OFF;

  void start(float base, int16_t ctrlMode, bool allowComp) {
    baseSec = max(1.0f, base); progress = 0; elapsed = 0; factor = 1; curveF = 1; reuseF = 1;
    ctrl = ctrlMode;
    compensate = allowComp && ctrlIsCurve(ctrlMode);
  }

  float curveF = 1;       // sam wspolczynnik temperatury
  float reuseF = 1;       // sam mnoznik zuzycia chemii

  // dt - sekundy od ostatniego wywolania, T - temperatura (NaN gdy brak czujnika),
  // reuse - mnoznik za zuzycie chemii (1 = swieza)
  void tick(float dt, float T, bool tValid, float reuse = 1.0f) {
    if (compensate && tValid) curveF = curveFactor(ctrlCurve(ctrl), T);
    // brak odczytu: zostaje ostatni znany wspolczynnik
    reuseF = reuse;
    factor = curveF * reuseF;
    progress += dt / (baseSec * factor);
    elapsed += dt;
  }

  bool done() const { return progress >= 1.0f; }
  float remaining() const { return max(0.0f, (1.0f - progress) * baseSec * factor); }
  float addedSec() const { return elapsed + remaining() - baseSec; }
};

// Czy temperatura jest w zakresie dla kroku (do koloru tla termometru)
bool tempInRange(int16_t ctrl, int16_t monMin10, int16_t monMax10, float T) {
  if (isnan(T)) return false;
  int16_t t10 = (int16_t)lroundf(T * 10);
  if (ctrl == CTRL_MON) return t10 >= monMin10 && t10 <= monMax10;
  if (ctrlIsCurve(ctrl)) return t10 >= ctrlCurve(ctrl).lo10 && t10 <= ctrlCurve(ctrl).hi10;
  return true;
}

// =====================================================================
//  MODEL STYGNIECIA CHEMII W KOREKSIE
//  (gdy sonda nie siega cieczy, np. Paterson na 3 rolki stojacy pionowo
//   z mala iloscia chemii - sonda mierzy wtedy temperature pokoju)
//
//  Zalozenia:
//   - chemia wlana w temperaturze nominalnej krzywej (np. 38,0 C)
//     do koreksu zagrzanego kapiela wstepna (scianki, spirale ~ 38 C),
//   - koreks stoi pionowo, spirale kreca sie w cieczy,
//   - pokoj = odczyt sondy w powietrzu na poczatku kroku, bez przeciagu.
//
//  Bilans cieplny (Euler, krok 1 s):   C * dT/dt = -UA(T) * (T - Tp)
//
//  C  = chemia (V * 4,18 J/ml/K) + zanurzona czesc scianek koreksu
//       (polistyren 1050 kg/m3, 1300 J/kg/K) + zanurzona czesc spiral i osi
//
//  UA = suma trzech drog ucieczki ciepla:
//   1) SCIANKA BOCZNA omywana przez ciecz (wysokosc slupa h = V / pole dna):
//        U = 1 / (1/h_wew + grubosc/k_scianki + 1/(h_zew + h_prom))
//        h_wew  - konwekcja w cieczy: spirale kreca sie -> ok. 300 W/m2K,
//                 STAND ok. 150, bez ruchu ok. 60
//        k      - polistyren 0,15 W/mK, scianka 2,2 mm
//        h_zew  - konwekcja naturalna powietrza na pionowej sciance,
//                 korelacja Churchilla-Chu:
//                   Ra = g*beta*dT*L^3 / (nu*alfa)
//                   Nu = (0,825 + 0,387*Ra^(1/6) / (1+(0,492/Pr)^(9/16))^(8/27))^2
//                   h  = Nu * k_pow / L         (L = h slupa cieczy)
//        h_prom - promieniowanie: eps*sigma*(Ts^2+Tp^2)*(Ts+Tp), eps = 0,92
//   2) DNO stojace na podstawie z PETG (szczelina powietrza w stopce
//      + slabo przewodzacy wydruk): ok. 3,5 W/m2K
//   3) GORA: lustro cieczy -> powietrze w koreksie (konwekcja od dolu
//      ok. 5 W/m2K) -> nieomywane scianki i pokrywa -> pokoj (szeregowo)
//
//  Geometria: Paterson Super System 4 na 3 rolki 35 mm (zmierz swoj!).
//  Spadek temperatury wydluza czas wg tej samej krzywej F(T) co pomiar:
//  krok konczy sie, gdy calka dt / (czas_bazowy * F(T(t))) osiagnie 1.
// =====================================================================
#define TANK_DI      0.100f   // srednica wewnetrzna koreksu [m]
#define TANK_WALL    0.0022f  // grubosc scianki [m]
#define TANK_HIN     0.175f   // wysokosc wnetrza do pokrywy [m]
#define K_WALL       0.15f    // polistyren [W/mK]
#define C_WALL_M2    (1050.0f * 1300.0f * TANK_WALL)   // pojemnosc cieplna 1 m2 scianki [J/K]
#define C_REELS      120.0f   // 3 spirale + os, calosc [J/K] (ok. 90 g plastiku)
#define H_BOTTOM     3.5f     // dno na podstawie z PETG [W/m2K]
#define H_SURF       5.0f     // lustro cieczy -> powietrze w koreksie [W/m2K]
#define EPS_TANK     0.92f    // emisyjnosc czarnego plastiku

static float hInner(int agit) { return agit == 0 || agit == 1 ? 300.0f : (agit == 2 ? 150.0f : 60.0f); }

// konwekcja naturalna na pionowej sciance (Churchill-Chu) + promieniowanie
static float hOuter(float Ts, float Ta, float L) {
  float dT = fabsf(Ts - Ta);
  if (L < 0.02f) L = 0.02f;
  float hr = EPS_TANK * 5.67e-8f * ((Ts + 273.15f) * (Ts + 273.15f) + (Ta + 273.15f) * (Ta + 273.15f)) *
             (Ts + Ta + 546.3f);
  if (dT < 0.05f) return hr + 1.0f;
  const float nu = 1.6e-5f, alfa = 2.25e-5f, kAir = 0.0264f, Pr = 0.71f;
  float beta = 1.0f / ((Ts + Ta) / 2 + 273.15f);
  float Ra = 9.81f * beta * dT * L * L * L / (nu * alfa);
  float den = powf(1.0f + powf(0.492f / Pr, 9.0f / 16.0f), 8.0f / 27.0f);
  float s = 0.825f + 0.387f * powf(Ra, 1.0f / 6.0f) / den;
  return s * s * kAir / L + hr;
}


static float hOuterHorizCyl(float Ts, float Ta, float D) {
  // konwekcja naturalna wokol poziomego walca (Churchill-Chu) + promieniowanie
  float dT = fabsf(Ts - Ta);
  float hr = EPS_TANK * 5.67e-8f * ((Ts + 273.15f) * (Ts + 273.15f) + (Ta + 273.15f) * (Ta + 273.15f)) *
             (Ts + Ta + 546.3f);
  if (dT < 0.05f) return hr + 1.0f;
  const float nu = 1.6e-5f, alfa = 2.25e-5f, kAir = 0.0264f, Pr = 0.71f;
  float beta = 1.0f / ((Ts + Ta) / 2 + 273.15f);
  float Ra = 9.81f * beta * dT * D * D * D / (nu * alfa);
  float den = powf(1.0f + powf(0.559f / Pr, 9.0f / 16.0f), 8.0f / 27.0f);
  float s = 0.60f + 0.387f * powf(Ra, 1.0f / 6.0f) / den;
  return s * s * kAir / D + hr;
}

// ---------------------------------------------------------------------
//  MODEL DWUWEZLOWY:  ciecz (Tl)  <->  scianki + spirale (Tw)  ->  pokoj (Tp)
//
//   Cl * dTl/dt = -Glw*(Tl - Tw) - UAla*(Tl - Tp)
//   Cw * dTw/dt = +Glw*(Tl - Tw) - UAwa*(Tw - Tp)
//
//   Cl   - chemia + woda, ktora zostala w koreksie po poprzedniej kapieli
//          (RESID_ML, ma temperature scianek)
//   Cw   - zanurzona scianka i dno (polistyren) + zanurzona czesc spiral
//   Glw  - ciecz <-> scianka i spirale (konwekcja w cieczy h_wew)
//   UAwa - scianka -> pokoj: przewodzenie przez scianke + konwekcja naturalna
//          (Churchill-Chu, liczona dla temperatury scianki) + promieniowanie;
//          dno przez podstawe z PETG
//   UAla - lustro cieczy -> powietrze w koreksie -> gora koreksu -> pokoj
//
//  Dzieki temu model obejmuje koreks NAGRZANY kapiela wstepna cieplejsza
//  niz chemia (np. 45 C): scianki najpierw oddaja cieplo chemii, potem
//  calosc powoli stygnie do pokoju.
// ---------------------------------------------------------------------
#define RESID_ML   15.0f     // woda pozostala na sciankach i spiralach po wylaniu [ml]
#define A_REELS    0.047f    // powierzchnia 3 spiral (obie strony) [m2]

struct CoolNet { float Cl, Cw, Glw, UAwa, UAla, hLiq; };

CoolNet coolNet(float Tl, float Tw, float Ta, int volMl, int agit, bool horiz) {
  CoolNet n;
  const float PI_ = 3.14159f;
  float r = TANK_DI / 2, aBase = PI_ * r * r, dOut = TANK_DI + 2 * TANK_WALL;
  float hi = hInner(agit);
  n.Cl = (volMl + RESID_ML) * 4.18f;
  if (!horiz) {
    float h = min(volMl * 1e-6f / aBase, TANK_HIN);
    float wet = h / TANK_HIN;
    float aSide = PI_ * dOut * h;
    n.Cw = (aSide + aBase) * C_WALL_M2 + C_REELS * wet;
    n.Glw = hi * (PI_ * TANK_DI * h + aBase) + hi * A_REELS * wet;
    float ho = hOuter(Tw, Ta, h);
    n.UAwa = aSide / (TANK_WALL / K_WALL + 1.0f / ho) + H_BOTTOM * aBase;
    float aUp = PI_ * dOut * (TANK_HIN - h) + aBase;
    float hoUp = hOuter((Tl + Ta) / 2, Ta, max(0.03f, TANK_HIN - h));
    float Uup = 1.0f / (1.0f / 4.0f + TANK_WALL / K_WALL + 1.0f / hoUp);
    n.UAla = 1.0f / (1.0f / (H_SURF * aBase) + 1.0f / (Uup * aUp));
    n.hLiq = h;
  } else {
    // koreks lezy: ciecz = odcinek kola; kat zwilzenia th z (th - sin th)/(2 pi) = wypelnienie
    float fill = min(1.0f, volMl * 1e-6f / (aBase * TANK_HIN));
    float lo = 0, hiA = 2 * PI_;
    for (int i = 0; i < 30; i++) { float m = (lo + hiA) / 2; if ((m - sinf(m)) / (2 * PI_) < fill) lo = m; else hiA = m; }
    float wet = lo / (2 * PI_);
    bool turning = agit == 0 || agit == 1;
    float wEff = turning ? wet + 0.5f * (1 - wet) : wet;      // obracanie: film cieczy na reszcie scianki (waga 0,5)
    float aSide = PI_ * dOut * TANK_HIN * wEff;
    float aEnds = aBase * (turning ? wEff : fill);
    n.Cw = (PI_ * dOut * TANK_HIN * wet + 2 * aBase * fill) * C_WALL_M2 + C_REELS * fill;
    n.Glw = hi * (aSide + 2 * aEnds) + hi * A_REELS * fill;
    float ho = hOuterHorizCyl(Tw, Ta, dOut);
    n.UAwa = aSide / (TANK_WALL / K_WALL + 1.0f / ho) + (H_BOTTOM + 3.5f) * aEnds;   // dno w gniezdzie + pokrywa
    n.UAla = 0;
    if (!turning) {
      float chord = 2 * r * sinf(lo / 2);
      float aUp = PI_ * dOut * TANK_HIN * (1 - wet);
      float Uup = 1.0f / (1.0f / 4.0f + TANK_WALL / K_WALL + 1.0f / ho);
      n.UAla = 1.0f / (1.0f / (H_SURF * chord * TANK_HIN) + 1.0f / (Uup * aUp));
    }
    n.hLiq = r * (1 - cosf(lo / 2));
  }
  return n;
}

// temperatura cieczy tuz po wlaniu: chemia miesza sie z woda pozostala w koreksie
float coolPourTemp(float Tchem, float Tw, int volMl) {
  return (volMl * Tchem + RESID_ML * Tw) / (volMl + RESID_ML);
}

// calkowanie o dt [s] (podkroki max 0,5 s - stabilnosc)
void coolStep(float &Tl, float &Tw, float Ta, int volMl, int agit, bool horiz, float dt) {
  while (dt > 0) {
    float h = min(dt, 0.5f);
    CoolNet n = coolNet(Tl, Tw, Ta, volMl, agit, horiz);
    float qlw = n.Glw * (Tl - Tw), qwa = n.UAwa * (Tw - Ta), qla = n.UAla * (Tl - Ta);
    Tl -= (qlw + qla) / n.Cl * h;
    Tw += (qlw - qwa) / n.Cw * h;
    dt -= h;
  }
}

// Rzeczywisty czas kroku [s]: chemia wlana w temperaturze nominalnej krzywej,
// scianki maja Tw0 (np. 45 C po kapieli wstepnej). Zwraca tez temperature
// po wlaniu (Tstart), najwyzsza (Tmax) i na koncu (Tend).
float coolStepTime(const Curve &cv, float base, float reuse, float Ta, int volMl, int agit, bool horiz,
                   float Tw0, float *Tend = nullptr, float *Tstart = nullptr, float *Tmax = nullptr) {
  float Tl = coolPourTemp(cv.nom10 / 10.0f, Tw0, volMl), Tw = Tw0;
  if (Tstart) *Tstart = Tl;
  float prog = 0, t = 0, mx = Tl;
  while (prog < 1.0f && t < 6000) {
    float T1 = Tl;
    coolStep(Tl, Tw, Ta, volMl, agit, horiz, 1.0f);
    mx = max(mx, Tl);
    prog += 1.0f / (base * curveFactor(cv, (T1 + Tl) / 2) * reuse);
    t += 1.0f;
  }
  if (Tend) *Tend = Tl;
  if (Tmax) *Tmax = mx;
  return t;
}

// ---------------------------------------------------------------------
//  PUSTY KOREKS (po wylaniu kapieli, gdy czekamy az chemia dojdzie do
//  temperatury): stygnie caly plastik + spirale + woda na sciankach.
//  Cala powierzchnia zewnetrzna oddaje cieplo (scianka + pokrywa,
//  dno przez podstawe z PETG).
//  Przyklad: 45 C, pokoj 21 C -> stala czasu ok. 8 min,
//  po 2 min koreks ma ok. 5 C mniej.
// ---------------------------------------------------------------------
float coolEmptyStep(float Tw, float Ta, bool horiz, float dt) {
  const float PI_ = 3.14159f;
  float r = TANK_DI / 2, aBase = PI_ * r * r, dOut = TANK_DI + 2 * TANK_WALL;
  float aSide = PI_ * dOut * TANK_HIN;
  float C = (aSide + 2 * aBase) * C_WALL_M2 + C_REELS + RESID_ML * 4.18f;
  while (dt > 0) {
    float h = min(dt, 1.0f);
    float ho = horiz ? hOuterHorizCyl(Tw, Ta, dOut) : hOuter(Tw, Ta, TANK_HIN);
    float UA = aSide / (TANK_WALL / K_WALL + 1.0f / ho)            // scianka boczna
             + aBase / (TANK_WALL / K_WALL + 1.0f / 4.0f + 1.0f / ho)  // pokrywa (lejek + pokrywka)
             + H_BOTTOM * aBase;                                    // dno na podstawie
    Tw -= UA * (Tw - Ta) / C * h;
    dt -= h;
  }
  return Tw;
}
