// =====================================================================
//  ui.h  -  interfejs dotykowy 320x240 (poziomo), styl Spool
// =====================================================================
#pragma once
#include "display.h"
#include "programs.h"
#include "thermo.h"
#include "bootio.h"
#include "ota.h"
#include "spool_logo.h"

// Wszystkie ekrany rysuja przez "tft", ktore wskazuje na biezacy cel rysowania:
// normalnie fizyczny wyswietlacz, a przy zrzucie ekranu (komenda 'P') bufor w RAM.
lgfx::LovyanGFX *G = &lcd;
#define tft (*G)

// Jezyk interfejsu: 0 = polski, 1 = English (zapisywany w pamieci jako "lang")
int16_t gLang = 0;
#define TR(pl, en) (gLang ? (en) : (pl))
static const char *const LANG_NAMES[] = {"PL", "EN"};

const char *stepLong(int16_t n) { return gLang ? STEP_LONG_EN[n] : STEP_LONG[n]; }
// nazwa do pigulki w naglowku: marka chemii + rodzaj procesu ("ADOX C-41"),
// bez zestawu chemii - tytul programu
void progBadge(int idx, char *out, size_t n);
// krotkie nazwy zestawow do naglowka przebiegu (kolejnosc jak KITS[])
static const char *const KIT_SHORT[] = {"", "ADOX 1L", "ADOX 0,5L", "Tetenal 1L", "Tetenal 0,5L",
                                         "Bellini", "Cinestill", "Ogolna"};
static_assert(sizeof(KIT_SHORT) / sizeof(KIT_SHORT[0]) == sizeof(KITS) / sizeof(KITS[0]), "KIT_SHORT");
const char *kitName(int16_t k) {
  if (gLang && k == KIT_NONE) return "No extension";
  if (gLang && k == NUM_KITS - 1) return "Generic +2%/roll";
  return KITS[k].name;
}

// ---------------------------------------------------------------------
//  Kolory i czcionki
// ---------------------------------------------------------------------
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
// Styl aplikacji Spool (Loupe Negative Lab): prawie czarne tlo, panele #121212,
// cienkie ramki #262626, tekst #e0e0e0, akcent pomaranczowy #d2661f,
// glowne przyciski jasne (#e9e9e5) z ciemnym tekstem
static const uint16_t C_BG     = rgb565(7, 7, 7);        // --bg-app
static const uint16_t C_TXT    = rgb565(224, 224, 224);  // --text-primary
static const uint16_t C_TXT2   = rgb565(168, 168, 168);  // --text-secondary
static const uint16_t C_ACC    = rgb565(210, 102, 31);   // --accent
static const uint16_t C_HDR    = rgb565(7, 7, 7);
static const uint16_t C_BAR    = rgb565(18, 18, 18);     // --bg-panel-raised
static const uint16_t C_BTN    = rgb565(18, 18, 18);     // panel / karta
static const uint16_t C_BTN2   = rgb565(28, 28, 28);     // przycisk drugorzedny (--bg-active)
static const uint16_t C_BORDER = rgb565(38, 38, 38);     // --border-default
static const uint16_t C_WHITE  = rgb565(233, 233, 229);
static const uint16_t C_DIM    = rgb565(135, 135, 135);  // --text-tertiary
static const uint16_t C_GREEN  = rgb565(233, 233, 229);  // glowny przycisk: bialy jak w Spool (--btn-primary-bg)
static const uint16_t C_OK     = rgb565(76, 175, 122);   // --success
static const uint16_t C_RED    = rgb565(190, 50, 45);    // STOP, usuwanie
static const uint16_t C_RED2   = rgb565(255, 85, 70);    // STOP po pierwszym dotknieciu (TR("PEWNO?", "SURE?"))
static const uint16_t C_REJ    = rgb565(92, 31, 31);     // --mark-reject (tlo "poza zakresem")
static const uint16_t C_TOK    = rgb565(18, 18, 18);

// kolory procesow z palety Spool: B&W, C-41, E-6, ECN-2, RA-4
static const uint16_t CAT_COL[] = {rgb565(156, 147, 140), rgb565(210, 102, 31), rgb565(76, 140, 226),
                                   rgb565(201, 79, 176), rgb565(226, 83, 76)};

int brightness565(uint16_t c) {
  int r = ((c >> 11) & 31) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
  return (r * 30 + g * 59 + b * 11) / 100;
}

// rozjasnienie koloru RGB565 (obwodki, efekt "wypuklego" przycisku)
uint16_t lighten(uint16_t c, uint8_t a) {
  int r = ((c >> 11) & 31) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
  r = min(255, r + a); g = min(255, g + a); b = min(255, b + a);
  return rgb565(r, g, b);
}

#define F_S    (&lgfx::fonts::FreeSans9pt7b)
#define F_SB   (&lgfx::fonts::FreeSansBold9pt7b)
#define F_M    (&lgfx::fonts::FreeSansBold12pt7b)
#define F_L    (&lgfx::fonts::FreeSansBold18pt7b)
#define F_MONO (&lgfx::fonts::FreeMonoBold9pt7b)
#define F_7SEG (&lgfx::fonts::Font7)

#define HDR_H 32
#define SCR_W 320
#define SCR_H 240

// ---------------------------------------------------------------------
//  Tekst z obsluga znaku stopnia: '~' rysowane jako kolko
//  align: 0 = do lewej, 1 = srodek, 2 = do prawej (y = srodek tekstu)
// ---------------------------------------------------------------------
void txt(lgfx::LovyanGFX *g, const char *s, int x, int y, uint8_t align, uint16_t col) {
  g->setTextColor(col);
  g->setTextDatum(lgfx::textdatum_t::middle_left);
  const char *d = strchr(s, '~');
  char a[64];
  size_t la = d ? (size_t)(d - s) : strlen(s);
  if (la > sizeof(a) - 1) la = sizeof(a) - 1;
  memcpy(a, s, la); a[la] = 0;
  const char *b = d ? d + 1 : "";
  int fh = g->fontHeight();
  int r = max(2, fh / 9);
  int degW = d ? r * 2 + 3 : 0;
  int wa = g->textWidth(a);
  int w = wa + degW + (d ? g->textWidth(b) : 0);
  int x0 = align == 0 ? x : (align == 1 ? x - w / 2 : x - w);
  g->drawString(a, x0, y);
  if (d) {
    int cx = x0 + wa + r + 1, cy = y - fh / 2 + r + fh / 6;
    g->drawCircle(cx, cy, r, col);
    if (r > 2) g->drawCircle(cx, cy, r - 1, col);
    g->drawString(b, x0 + wa + degW, y);
  }
}

// ---------------------------------------------------------------------
//  Sprite do odswiezania fragmentow bez migotania
// ---------------------------------------------------------------------
lgfx::LGFX_Sprite spr(&lcd);
uint16_t boxBg = 0;               // tlo biezacego panelu (dla przewijanych napisow)
int16_t boxW = 0, boxH = 0;
void boxFlushMarq(int x, int y);
lgfx::LovyanGFX *boxBegin(int w, int h, uint16_t bg) {
  spr.setColorDepth(16);
  if (!spr.createSprite(w, h)) return nullptr;
  spr.fillSprite(bg);
  boxBg = bg; boxW = w; boxH = h;
  return &spr;
}
void boxEnd(int x, int y) { spr.pushSprite(G, x, y); spr.deleteSprite(); boxFlushMarq(x, y); }

// ---------------------------------------------------------------------
//  Przewijany tekst ("marquee"): gdy napis nie miesci sie w swoim polu,
//  jest rysowany w okienku i plynnie przesuwa sie w lewo (pauza na
//  poczatku), wiec widac go w calosci i nic na nic nie nachodzi.
//   - teksty statyczne: txtFit() przy pelnym rysowaniu ekranu,
//   - teksty w odswiezanych panelach (sprite): txtP() - pole zgloszone
//     w panelu jest rejestrowane w boxEnd() ze wspolrzednymi ekranu.
//  Przesuniecie liczone jest z zegara od pierwszego pojawienia sie
//  napisu, wiec panel odswiezany co 250 ms i marqueeTick() co 40 ms
//  rysuja dokladnie to samo - bez migotania.
// ---------------------------------------------------------------------
struct Marquee {
  int16_t x, y, w, h, tw;
  const lgfx::IFont *font;
  uint16_t col, bg;
  uint32_t t0, key;
  bool inBox;                 // z panelu (odswiezany) - usuwany, gdy panel go nie zglosi
  char s[64];
};
#define MAX_MARQ 12
Marquee marq[MAX_MARQ];
uint8_t nMarq = 0;
Marquee marqPend[6];          // zgloszone w biezacym panelu (wspolrzedne w panelu)
uint8_t nPend = 0;
#define MARQ_GAP 40           // odstep miedzy koncem a kolejnym poczatkiem napisu
#define MARQ_PAUSE 1500       // ms postoju na poczatku
#define MARQ_SPEED 30         // px na sekunde

static uint32_t marqHash(const char *s) { uint32_t h = 2166136261u; while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; } return h; }
// t0 napisu: jesli juz przewija sie gdzies taki sam tekst - ten sam czas (ciaglosc)
static uint32_t marqT0(uint32_t key) {
  for (int i = 0; i < nMarq; i++) if (marq[i].key == key) return marq[i].t0;
  for (int i = 0; i < nPend; i++) if (marqPend[i].key == key) return marqPend[i].t0;
  return millis();
}
static int marqOffset(uint32_t t0, int tw) {
  uint32_t cycle = MARQ_PAUSE + (uint32_t)(tw + MARQ_GAP) * 1000 / MARQ_SPEED;
  uint32_t ph = (millis() - t0) % cycle;
  return ph < MARQ_PAUSE ? 0 : (int)((ph - MARQ_PAUSE) * MARQ_SPEED / 1000);
}
// rysuje przewiniety napis w prostokacie (x,y,w,h) na g (przyciete)
static void marqPaint(lgfx::LovyanGFX *g, const Marquee &m, int x, int y, int off) {
  g->setClipRect(x, y, m.w, m.h);
  g->fillRect(x, y, m.w, m.h, m.bg);
  g->setFont(m.font);
  txt(g, m.s, x - off, y + m.h / 2, 0, m.col);
  if (off > 0) txt(g, m.s, x - off + m.tw + MARQ_GAP, y + m.h / 2, 0, m.col);
  g->clearClipRect();
}
static void marqDraw(const Marquee &m) {
  auto g = boxBegin(m.w, m.h, m.bg);
  if (!g) return;
  marqPaint(g, m, 0, 0, marqOffset(m.t0, m.tw));
  spr.pushSprite(G, m.x, m.y); spr.deleteSprite();
}

// jak txt(), ale z maksymalna szerokoscia maxW; bg = kolor tla pod napisem.
// Uzywa aktualnej czcionki tft (teksty statyczne).
void txtFit(const char *s, int x, int y, uint8_t align, uint16_t col, int maxW, uint16_t bg = C_BG) {
  int tw = tft.textWidth(s);
  int mx = align == 0 ? x : (align == 1 ? x - maxW / 2 : x - maxW);
  int mh = tft.fontHeight() + 4, my = y - mh / 2;
  // ten sam obszar rysowany ponownie (np. odswiezony wiersz) - usun stary wpis
  for (int i = 0; i < nMarq; i++)
    if (marq[i].x == mx && marq[i].y == my) { marq[i] = marq[--nMarq]; break; }
  if (tw <= maxW || maxW < 20) { txt(&tft, s, x, y, align, col); return; }
  Marquee m;
  m.x = mx; m.h = mh; m.y = my;
  m.w = maxW; m.tw = tw;
  m.font = tft.getFont();
  m.col = col; m.bg = bg; m.key = marqHash(s); m.t0 = marqT0(m.key); m.inBox = false;
  strncpy(m.s, s, sizeof(m.s) - 1); m.s[sizeof(m.s) - 1] = 0;
  marqDraw(m);
  if (nMarq < MAX_MARQ && G == &lcd) marq[nMarq++] = m;
}

// txt() w panelu (sprite): szerokosc dostepna liczona z polozenia i
// szerokosci panelu; gdy napis sie nie miesci - przewija sie.
void txtP(lgfx::LovyanGFX *g, const char *s, int x, int y, uint8_t align, uint16_t col, int maxW = 0) {
  if (g != &spr) { txt(g, s, x, y, align, col); return; }
  if (maxW <= 0) maxW = align == 0 ? boxW - x - 2 : (align == 1 ? 2 * min(x, boxW - x) - 4 : x - 2);
  int tw = g->textWidth(s);
  if (tw <= maxW || maxW < 20) { txt(g, s, x, y, align, col); return; }
  Marquee m;
  m.w = maxW; m.tw = tw;
  m.h = min((int)g->fontHeight() + 4, (int)boxH);
  m.x = align == 0 ? x : (align == 1 ? x - maxW / 2 : x - maxW);
  m.y = max(0, y - m.h / 2);
  m.font = g->getFont();
  m.col = col; m.bg = boxBg; m.key = marqHash(s); m.t0 = marqT0(m.key); m.inBox = true;
  strncpy(m.s, s, sizeof(m.s) - 1); m.s[sizeof(m.s) - 1] = 0;
  marqPaint(g, m, m.x, m.y, marqOffset(m.t0, m.tw));
  if (nPend < 6) marqPend[nPend++] = m;
}
// po wypchnieciu panelu: odswiez liste przewijanych napisow w jego obszarze
void boxFlushMarq(int x, int y) {
  for (int i = 0; i < nMarq;) {                       // stare wpisy z tego panelu
    Marquee &m = marq[i];
    if (m.inBox && m.x >= x && m.y >= y && m.x < x + boxW && m.y < y + boxH) marq[i] = marq[--nMarq];
    else i++;
  }
  if (G == &lcd)
    for (int i = 0; i < nPend && nMarq < MAX_MARQ; i++) {
      Marquee m = marqPend[i];
      m.x += x; m.y += y;
      marq[nMarq++] = m;
    }
  nPend = 0;
}

void marqueeTick() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (!nMarq || now - last < 40) return;
  last = now;
  for (int i = 0; i < nMarq; i++) marqDraw(marq[i]);
}

// ---------------------------------------------------------------------
//  Formatowanie
// ---------------------------------------------------------------------
void fmtTime(char *b, size_t n, int sec) {
  if (sec < 0) sec = 0;
  if (sec > 5999) sec = 5999;
  snprintf(b, n, "%02d:%02d", sec / 60, sec % 60);
}
float toUnit(float c) { return S.tempF ? c * 9.0f / 5.0f + 32.0f : c; }
void fmtTemp(char *b, size_t n, float c, bool valid) {
  if (!valid || isnan(c)) snprintf(b, n, "--.-~%c", S.tempF ? 'F' : 'C');
  else snprintf(b, n, "%.1f~%c", toUnit(c), S.tempF ? 'F' : 'C');
}

// ---------------------------------------------------------------------
//  Rejestr przyciskow ekranu
// ---------------------------------------------------------------------
struct Btn { int16_t x, y, w, h; uint16_t id; bool rep; };
Btn btns[48];
uint8_t nBtns = 0;

void clearBtns() { nBtns = 0; }
void addBtn(int x, int y, int w, int h, uint16_t id, bool rep = false) {
  for (uint8_t i = 0; i < nBtns; i++)
    if (btns[i].id == id) { btns[i] = Btn{(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, id, rep}; return; }
  if (nBtns < 48) btns[nBtns++] = Btn{(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, id, rep};
}
void removeBtn(uint16_t id) {
  for (uint8_t i = 0; i < nBtns; i++)
    if (btns[i].id == id) { btns[i] = btns[--nBtns]; return; }
}
int hitBtn(int x, int y) {
  for (uint8_t i = 0; i < nBtns; i++) {
    const Btn &b = btns[i];
    if (x >= b.x - 3 && x < b.x + b.w + 3 && y >= b.y - 3 && y < b.y + b.h + 3) return i;
  }
  return -1;
}
void button(uint16_t id, int x, int y, int w, int h, const char *label, uint16_t bg,
            uint16_t fg = C_WHITE, const lgfx::IFont *f = F_SB, bool rep = false) {
  // plaski przycisk jak w Spool; na jasnym tle (przycisk glowny) tekst ciemny
  bool light = brightness565(bg) > 160;
  if (light && fg == C_WHITE) fg = rgb565(10, 10, 10);
  tft.fillRoundRect(x, y, w, h, 8, bg);
  if (!light) tft.drawRoundRect(x, y, w, h, 8, bg == C_BTN2 ? rgb565(51, 51, 51) : lighten(bg, 30));
  tft.setFont(f);
  txtFit(label, x + w / 2, y + h / 2, 1, fg, w - 10, bg);
  addBtn(x, y, w, h, id, rep);
}

// karta (tlo elementu listy) z cienkim kolorowym paskiem po lewej
void card(int x, int y, int w, int h, uint16_t stripe) {
  tft.fillRoundRect(x, y, w, h, 8, C_BTN);
  tft.drawRoundRect(x, y, w, h, 8, C_BORDER);
  if (stripe) tft.fillRoundRect(x + 4, y + 6, 3, h - 12, 1, stripe);
}

// ---------------------------------------------------------------------
//  Identyfikatory przyciskow
// ---------------------------------------------------------------------
enum : uint16_t {
  B_BACK = 1,
  B_MAIN0 = 10,                     // 10..13
  B_LIST0 = 20, B_LIST_UP = 30, B_LIST_DN = 31,
  B_PROW0 = 40, B_PADD = 59, B_PUP = 60, B_PDN = 61, B_PRESET = 62, B_PSTART = 64,
  B_MUP = 70, B_MDN = 71,
  B_RSTART = 80, B_RPAUSE = 81, B_RSKIP = 82, B_RSTOP = 83, B_RNEXT = 84, B_ROK = 85, B_RTEMP = 86,
  B_TRESET = 90,
  B_MRPMM = 100, B_MRPMP = 101, B_MCURM = 102, B_MCURP = 103, B_MDIR = 104, B_MGO = 105,
  B_CDONE = 110, B_CAGAIN = 111,
  B_YES = 120, B_NO = 121,
  B_CFRESH = 130, B_CUSED = 131, B_CRM = 132, B_CRP = 133, B_CKM = 134, B_CKP = 135,
  B_CGO = 136, B_CCANCEL = 137, B_DTM = 138, B_DTP = 139,
  B_CAT0 = 140,                     // 140..145 kafle rodzajow procesu
  B_SUV = 150, B_SUH = 151, B_SUPY = 152, B_SUPN = 153, B_SUVM = 154, B_SUVP = 155, B_SUGO = 156, B_SUTM = 157, B_SUTP = 158,
  B_ITEM0 = 1000                   // 1000 + wiersz*4 + {0:-, 1:+, 2:przycisk}
};

// ---------------------------------------------------------------------
//  Ekrany i stan
// ---------------------------------------------------------------------
enum Scr : uint8_t { S_MAIN, S_LIST, S_PROG, S_MENU, S_RUN, S_THERMO, S_MOTOR, S_CALIB,
                     S_OTA, S_INFO, S_CONFIRM, S_NOTOUCH, S_CAT, S_CHEM, S_SETUP };
Scr scr = S_MAIN;
bool fullRedraw = true;
uint16_t heldId = 0;
uint16_t repeatCount = 0;
uint32_t touchLockUntil = 0;   // 0 = brak blokady dotyku
void touchLock(uint32_t ms) { touchLockUntil = millis() + ms; if (!touchLockUntil) touchLockUntil = 1; }
extern bool calTest;
void calibAccept();
bool settingsDirty = false, progDirty = false;

enum ListMode : uint8_t { L_DEFAULT, L_CUSTOM, L_COPY };
ListMode listMode = L_DEFAULT;
int listCat = -1;          // -1 = wszystkie rodzaje
int listPage = 0;
int curProg = 0;
int progScroll = 0;
int curStep = 0;

enum MenuId : uint8_t { M_SETTINGS, M_ROLL, M_STICK, M_STAND, M_BWRED, M_RESET, M_STEP, M_TSRC };
void openMenu(MenuId m);
extern int16_t runNoTherm, runVolMl;
MenuId menuId = M_SETTINGS;
int menuPage = 0;

// potwierdzenie
enum ConfirmAct : uint8_t { CA_RESET_PROG, CA_RESET_DEFAULTS, CA_RESET_CUSTOM, CA_RESET_SETTINGS,
                            CA_FACTORY, CA_DEL_STEP };
ConfirmAct confirmAct;
char confirmText[48];
Scr confirmReturn;

void goScreen(Scr s) {
  if (scr == S_OTA && s != S_OTA) otaStop();
  if ((scr == S_MENU || scr == S_MOTOR) && s != scr) {
    if (settingsDirty) { saveSettings(); settingsDirty = false; }
    if (progDirty) { saveProgram(curProg); progDirty = false; }
  }
  if (scr == S_MOTOR && s != S_MOTOR) { motor.setTarget(0); }
  scr = s;
  fullRedraw = true;
  heldId = 0;
}
void runStart(int prog);
// ekran po wlaczeniu: ostatnio wybrany program (ekran START), cofniecie -> lista
void goStartScreen() {
  int p = prefs.getShort("lastP", -1);
  int c = prefs.getShort("lastC", -1);
  if (p >= 0 && p < NUM_PROGS && progs[p].nSteps > 0) {
    curProg = p; progScroll = 0;
    listCat = (c >= -1 && c < 6) ? c : -1;
    listPage = 0;
    runStart(p);                 // od razu "WLEJ: pierwszy krok"; STOP -> program -> lista
  } else goScreen(S_MAIN);
}
void askConfirm(ConfirmAct a, const char *text) {
  confirmAct = a;
  strncpy(confirmText, text, sizeof(confirmText) - 1);
  confirmText[sizeof(confirmText) - 1] = 0;
  confirmReturn = scr;
  goScreen(S_CONFIRM);
}

// ---------------------------------------------------------------------
//  Naglowek
// ---------------------------------------------------------------------
bool hdrTemp = false;
void drawHeaderTemp() {
  if (!hdrTemp) return;
  char b[16];
  fmtTemp(b, sizeof(b), thermo.t, thermo.valid);
  auto g = boxBegin(80, HDR_H - 2, C_HDR);
  if (!g) return;
  g->setFont(F_SB);
  txtP(g, b, 76, HDR_H / 2 - 1, 2, thermo.valid ? C_WHITE : C_DIM);
  boxEnd(SCR_W - 80, 0);
}
uint16_t hdrAccent = C_ACC;     // kolor odznaki programu (kolor procesu)
void drawHeader(const char *title, bool back, const char *pnum = nullptr, bool showTemp = true) {
  tft.fillRect(0, 0, SCR_W, HDR_H, C_HDR);
  tft.drawFastHLine(0, HDR_H - 1, SCR_W, C_BORDER);          // cienka linia jak w Spool
  int x = 12;
  if (back) {
    for (int d = 0; d < 2; d++) {          // cienki szewron "<"
      tft.drawLine(24 + d, 8, 16 + d, 15, C_TXT2);
      tft.drawLine(16 + d, 15, 24 + d, 22, C_TXT2);
    }
    tft.drawFastVLine(40, 8, HDR_H - 16, C_BORDER);
    addBtn(0, 0, 58, HDR_H + 4, B_BACK);
    x = 50;
  }
  if (pnum) {                                   // pigulka z nazwa chemii i procesu, np. "ADOX C-41"
    tft.setFont(F_SB);
    int pw = min(150, (int)tft.textWidth(pnum) + 16);
    tft.fillRoundRect(x - 2, 5, pw, HDR_H - 12, 6, hdrAccent);
    txtFit(pnum, x - 2 + pw / 2, HDR_H / 2 - 1, 1, C_BG, pw - 12, hdrAccent);
    x += pw + 6;
  }
  tft.setFont(F_SB);
  if (title[0]) txtFit(title, x, HDR_H / 2, 0, C_TXT, (showTemp ? SCR_W - 84 : SCR_W - 6) - x, C_HDR);
  hdrTemp = showTemp;
  drawHeaderTemp();
}

// ---------------------------------------------------------------------
//  Strzalki przewijania (prawa kolumna)
// ---------------------------------------------------------------------
// pos / total - ktora strona listy jest widoczna (np. "2/3") - wypisane miedzy strzalkami
void drawScrollArrows(uint16_t idUp, uint16_t idDn, int y0, int y1, bool canUp, bool canDn,
                      int pos = 0, int total = 0) {
  int x = 284, w = 34, mid = (y0 + y1) / 2;
  const int gap = 13;                                   // miejsce na "2/3"
  tft.fillRect(x, y0, w, y1 - y0, C_BG);
  tft.fillRoundRect(x, y0, w, mid - gap - y0, 8, canUp ? C_BTN2 : C_BTN);
  tft.fillRoundRect(x, mid + gap, w, y1 - mid - gap, 8, canDn ? C_BTN2 : C_BTN);
  tft.drawRoundRect(x, y0, w, mid - gap - y0, 8, C_BORDER);
  tft.drawRoundRect(x, mid + gap, w, y1 - mid - gap, 8, C_BORDER);
  int cx = x + w / 2, a = (y0 + mid - gap) / 2, b = (mid + gap + y1) / 2;
  uint16_t cu = canUp ? C_TXT : rgb565(60, 60, 60), cd = canDn ? C_TXT : rgb565(60, 60, 60);
  tft.fillTriangle(cx, a - 7, cx - 8, a + 5, cx + 8, a + 5, cu);
  tft.fillTriangle(cx, b + 7, cx - 8, b - 5, cx + 8, b - 5, cd);
  if (total > 0) {
    char s[12];
    snprintf(s, sizeof(s), "%d/%d", pos, total);
    tft.setFont(F_SB);
    txt(&tft, s, cx, mid, 1, C_ACC);
  }
  addBtn(x, y0, w, mid - y0, idUp, true);
  addBtn(x, mid, w, y1 - mid, idDn, true);
}

// =====================================================================
//  EKRAN: menu glowne
// =====================================================================
// --- proste ikony rysowane wektorowo (cx, cy = srodek ikony) ---
void iconFilm(int cx, int cy, uint16_t c) {           // kaseta filmu 35 mm
  tft.fillRoundRect(cx - 16, cy - 14, 22, 30, 4, c);
  tft.fillRect(cx - 12, cy - 19, 14, 6, c);
  tft.fillRect(cx + 6, cy - 6, 16, 18, c);
  for (int i = 0; i < 3; i++) tft.fillRect(cx + 9 + i * 5, cy - 3, 3, 3, C_BTN);
  for (int i = 0; i < 3; i++) tft.fillRect(cx + 9 + i * 5, cy + 6, 3, 3, C_BTN);
}
void iconStar(int cx, int cy, uint16_t c) {           // gwiazdka = wlasne
  float r1 = 18, r2 = 7.5f;
  int px[10], py[10];
  for (int i = 0; i < 10; i++) {
    float a = -1.5708f + i * 0.6283f, r = (i & 1) ? r2 : r1;
    px[i] = cx + (int)(r * cosf(a)); py[i] = cy + (int)(r * sinf(a));
  }
  for (int i = 0; i < 10; i++) tft.fillTriangle(cx, cy, px[i], py[i], px[(i + 1) % 10], py[(i + 1) % 10], c);
}
void iconThermo(int cx, int cy, uint16_t c) {
  tft.fillRoundRect(cx - 5, cy - 20, 10, 30, 5, c);
  tft.fillCircle(cx, cy + 12, 9, c);
  tft.fillRoundRect(cx - 2, cy - 8, 4, 20, 2, C_RED);
  tft.fillCircle(cx, cy + 12, 6, C_RED);
}
void iconGear(int cx, int cy, uint16_t c) {
  for (int i = 0; i < 8; i++) {
    float a = i * 0.7854f;
    tft.fillCircle(cx + (int)(15 * cosf(a)), cy + (int)(15 * sinf(a)), 5, c);
  }
  tft.fillCircle(cx, cy, 14, c);
  tft.fillCircle(cx, cy, 6, C_BTN);
}

// duzy kafel z ikona i podpisem
void tile(uint16_t id, int x, int y, int w, int h, const char *label, const char *sub,
          uint16_t accent, uint8_t icon) {
  tft.fillRoundRect(x, y, w, h, 8, C_BTN);
  tft.drawRoundRect(x, y, w, h, 8, C_BORDER);
  int cx = x + w / 2, cy = y + 34;
  switch (icon) {
    case 0: iconFilm(cx, cy, accent); break;
    case 1: iconStar(cx, cy, accent); break;
    case 2: iconThermo(cx, cy, accent); break;
    case 3: iconGear(cx, cy, accent); break;
  }
  tft.setFont(F_SB);
  txt(&tft, label, cx, y + h - 26, 1, C_WHITE);
  if (sub) { tft.setFont(&lgfx::fonts::Font0); txt(&tft, sub, cx, y + h - 12, 1, C_DIM); }
  addBtn(x, y, w, h, id);
}

void drawMain() {
  hdrAccent = C_TXT;
  drawHeader("", false);
  tft.drawBitmap(12, (HDR_H - SPOOL_SMALL_H) / 2, SPOOL_SMALL, SPOOL_SMALL_W, SPOOL_SMALL_H, C_TXT);
  tft.setFont(&lgfx::fonts::Font0);
  txt(&tft, "FILM PROCESSOR", 100, HDR_H / 2, 0, C_DIM);
  tile(B_MAIN0 + 0, 6, 38, 151, 98, TR("FABRYCZNE", "FACTORY"), TR("gotowe programy", "ready programs"), C_ACC, 0);
  tile(B_MAIN0 + 1, 163, 38, 151, 98, TR("WLASNE", "CUSTOM"), TR("Twoje programy", "your programs"), C_TXT, 1);
  tile(B_MAIN0 + 2, 6, 140, 151, 98, TR("TERMOMETR", "THERMOMETER"), TR("podglad chemii", "chemistry temp"), C_TXT, 2);
  tile(B_MAIN0 + 3, 163, 140, 151, 98, TR("USTAWIENIA", "SETTINGS"), TR("agitacja, ekran", "agitation, screen"), C_TXT2, 3);
}

// =====================================================================
//  EKRAN: wybor rodzaju procesu (C-41, B&W, E-6, ...)
// =====================================================================
int countPrograms(bool custom, int cat) {
  int n = 0;
  for (int i = 0; i < NUM_PROGS; i++)
    if (isCustom(i) == custom && (cat < 0 || progs[i].cat == cat)) n++;
  return n;
}

void catTile(uint16_t id, int x, int y, int w, int h, const char *label, int count, uint16_t col) {
  // kafel w stylu Spool: ciemny panel, kolorowy znacznik procesu, nazwa i liczba programow
  bool en = count > 0;
  tft.fillRoundRect(x, y, w, h, 8, C_BTN);
  tft.drawRoundRect(x, y, w, h, 8, en ? C_BORDER : rgb565(24, 24, 24));
  tft.fillRoundRect(x + 10, y + 12, 22, 6, 3, en ? col : C_BORDER);       // znacznik koloru
  tft.setFont(strlen(label) > 6 ? F_SB : F_M);                           // dluzsze napisy mniejsza czcionka
  txt(&tft, label, x + 10, y + h / 2 + 2, 0, en ? C_TXT : C_DIM);
  char b[20];
  if (gLang) snprintf(b, sizeof(b), count == 1 ? "%d program" : "%d programs", count);
  else snprintf(b, sizeof(b), count == 1 ? "%d program" : (count >= 2 && count <= 4 ? "%d programy" : "%d programow"), count);
  tft.setFont(&lgfx::fonts::Font0);
  txt(&tft, b, x + 10, y + h - 14, 0, en ? C_TXT2 : C_DIM);
  if (en) addBtn(x, y, w, h, id);
}

void drawCat() {
  hdrAccent = C_TXT;
  drawHeader(listMode == L_CUSTOM ? TR("WLASNE", "CUSTOM") : TR("FABRYCZNE", "FACTORY"), true);
  bool cu = listMode == L_CUSTOM;
  // kolejnosc kafli: C-41 (najczesciej), B&W, E-6 / ECN-2, RA-4, wszystkie
  static const int8_t ORDER[6] = {CAT_C41, CAT_BW, CAT_E6, CAT_ECN2, CAT_RA4, -1};
  for (int i = 0; i < 6; i++) {
    int x = 6 + (i % 3) * 104, y = 38 + (i / 3) * 101;
    int c = ORDER[i];
    catTile(B_CAT0 + i, x, y, 100, 97, c < 0 ? TR("Wszystkie", "All") : CAT_NAMES[c], countPrograms(cu, c),
            c < 0 ? C_BTN2 : CAT_COL[c]);
  }
}
int catFromTile(int i) {
  static const int8_t ORDER[6] = {CAT_C41, CAT_BW, CAT_E6, CAT_ECN2, CAT_RA4, -1};
  return ORDER[constrain(i, 0, 5)];
}

// =====================================================================
//  EKRAN: lista programow
// =====================================================================
#define LIST_ROWS 4
int listIdx[NUM_PROGS];
int listN = 0;

// lista programow do pokazania: rodzaj procesu + fabryczne/wlasne, posortowane wg rodzaju
void buildList() {
  listN = 0;
  for (int c = 0; c < NUM_CATS; c++)
    for (int i = 0; i < NUM_PROGS; i++) {
      if (progs[i].cat != c) continue;
      if (listMode == L_DEFAULT && isCustom(i)) continue;
      if (listMode == L_CUSTOM && !isCustom(i)) continue;
      if (listMode != L_COPY && listCat >= 0 && progs[i].cat != listCat) continue;
      listIdx[listN++] = i;
    }
}

int progTotalSec(int idx) {
  const Program &p = progs[idx];
  float t = 0;
  for (int i = 0; i < p.nSteps; i++) t += stepNominalSec(idx, p.s[i]);
  return (int)t;
}

// powyzej wydajnosci podanej przez producenta - tylko ostrzezenie, bez blokady
bool chemOverMax(const ChemState &c) { return KITS[c.kit].maxRolls && c.used > KITS[c.kit].maxRolls; }

// maly pomaranczowy znacznik "!" (srodek cx, cy)
void warnBadge(lgfx::LovyanGFX *g, int cx, int cy) {
  g->fillCircle(cx, cy, 7, C_ACC);
  g->fillRect(cx - 1, cy - 4, 3, 5, C_BG);
  g->fillRect(cx - 1, cy + 3, 3, 2, C_BG);
}

void progBadge(int idx, char *out, size_t n) {
  int k = chem[idx].kit;
  if (k != KIT_NONE) {
    char brand[16];
    snprintf(brand, sizeof(brand), "%s", KIT_SHORT[k]);
    char *sp = strchr(brand, ' '); if (sp) *sp = 0;          // pierwsze slowo: ADOX, Tetenal...
    snprintf(out, n, "%s %s", brand, CAT_NAMES[progs[idx].cat]);
  } else snprintf(out, n, "%s", progs[idx].title);
}

void drawList() {
  buildList();
  char t[32];
  if (listMode == L_COPY) snprintf(t, sizeof(t), "KOPIUJ Z...");
  else snprintf(t, sizeof(t), "%s  %s", listMode == L_DEFAULT ? TR("FABRYCZNE", "FACTORY") : TR("WLASNE", "CUSTOM"),
                listCat >= 0 ? CAT_NAMES[listCat] : TR("WSZYSTKIE", "ALL"));
  hdrAccent = (listCat >= 0 && listMode != L_COPY) ? CAT_COL[listCat] : C_TXT;
  drawHeader(t, true);
  int n = listN, pages = (n + LIST_ROWS - 1) / LIST_ROWS;
  listPage = constrain(listPage, 0, max(0, pages - 1));
  bool scroll = n > LIST_ROWS;
  int w = scroll ? 276 : 312;
  if (n == 0) {
    tft.setFont(F_SB);
    txt(&tft, TR("Brak programow tego rodzaju", "No programs of this type"), 160, 110, 1, C_DIM);
    tft.setFont(F_S);
    txt(&tft, TR("Wlasne: wejdz w WSZYSTKIE i uzyj KOPIUJ", "Custom: open ALL and use COPY"), 160, 140, 1, C_DIM);
  }
  for (int r = 0; r < LIST_ROWS; r++) {
    int k = listPage * LIST_ROWS + r;
    if (k >= n) break;
    int idx = listIdx[k];
    const Program &p = progs[idx];
    int y = 38 + r * 50;                              // 4 wysokie wiersze - latwo trafic palcem
    card(4, y, w, 46, CAT_COL[p.cat]);
    char line[48], tt[8];
    tft.setFont(F_SB);
    fmtTime(tt, sizeof(tt), progTotalSec(idx));
    txt(&tft, tt, w - 8, y + 16, 2, C_TXT2);
    txtFit(p.title, 16, y + 16, 0, C_TXT, w - 8 - tft.textWidth(tt) - 10 - 16, C_BTN);
    // druga linia: zestaw chemii i zuzycie
    tft.setFont(&lgfx::fonts::Font0);
    if (chem[idx].kit != KIT_NONE) {
      snprintf(line, sizeof(line), TR("%s - zuzyte %d", "%s - used %d"), kitName(chem[idx].kit), chem[idx].used);
      bool ov = chemOverMax(chem[idx]);
      int lmax = w - 16 - (ov ? 26 : 8);
      txtFit(line, 16, y + 34, 0, C_DIM, lmax, C_BTN);
      if (ov) warnBadge(&tft, 16 + min(lmax, (int)tft.textWidth(line)) + 10, y + 34);
    } else txt(&tft, CAT_NAMES[p.cat], 16, y + 34, 0, C_DIM);
    addBtn(4, y, w, 46, B_LIST0 + r);
  }
  if (scroll) drawScrollArrows(B_LIST_UP, B_LIST_DN, 38, 234, listPage > 0, listPage < pages - 1,
                               listPage + 1, pages);
}

// =====================================================================
//  EKRAN: przeglad programu (lista krokow)
// =====================================================================
#define PROW 4

void drawProgRow(int i) {
  const Program &p = progs[curProg];
  int k = progScroll + i;
  int y = 36 + i * 42;                               // 4 wysokie wiersze
  bool scroll = p.nSteps + 1 > PROW;
  int w = scroll ? 278 : 314;
  tft.fillRect(0, y, 282 + (scroll ? 0 : 38), 41, C_BG);
  if (k > p.nSteps) return;
  if (k == p.nSteps) {               // wiersz "dodaj krok"
    if (p.nSteps >= MAX_STEPS) return;
    tft.drawRoundRect(3, y, w, 38, 8, C_BORDER);
    tft.setFont(F_SB);
    txt(&tft, TR("+ DODAJ KROK", "+ ADD STEP"), 3 + w / 2, y + 19, 1, C_DIM);
    addBtn(3, y, w, 38, B_PADD);
    return;
  }
  const Step &s = p.s[k];
  card(3, y, w, 38, kitStepType(s) == KS_DEV ? CAT_COL[p.cat] : C_BORDER);
  char b[16];
  tft.setFont(F_MONO);
  txt(&tft, STEP_SHORT[s.name], 14, y + 19, 0, C_TXT);
  fmtTime(b, sizeof(b), s.timeSec);
  txt(&tft, b, 82, y + 19, 0, C_TXT);
  { int nch = rinseChg[curProg][k]; char ag[12];
    if (nch > 0) snprintf(ag, sizeof(ag), "%dx %s", nch, AGIT_NAMES[s.agit]); else snprintf(ag, sizeof(ag), "%s", AGIT_NAMES[s.agit]);
    txt(&tft, ag, 156, y + 19, 0, nch > 0 ? C_ACC : (s.agit == AG_NONE ? C_DIM : C_TXT2)); }
  txt(&tft, ctrlName(s.ctrl), w - 6, y + 19, 2, s.ctrl == CTRL_OFF ? C_DIM : (ctrlIsCurve(s.ctrl) ? CAT_COL[p.cat] : C_TXT2));
  addBtn(3, y, w, 38, B_PROW0 + i);
}

void drawProg() {
  const Program &p = progs[curProg];
  char title[40];
  snprintf(title, sizeof(title), "%s", p.title);
  hdrAccent = CAT_COL[p.cat];
  drawHeader(title, true);
  int rows = p.nSteps + 1;
  progScroll = constrain(progScroll, 0, max(0, rows - PROW));
  for (int i = 0; i < PROW; i++) drawProgRow(i);
  if (rows > PROW) drawScrollArrows(B_PUP, B_PDN, 36, 202, progScroll > 0, progScroll < rows - PROW,
                                    progScroll + 1, rows - PROW + 1);
  button(B_PRESET, 4, 206, 96, 32, isCustom(curProg) ? TR("KOPIUJ", "COPY") : "RESET", C_BTN2, C_WHITE);
  char t2[8];
  fmtTime(t2, sizeof(t2), progTotalSec(curProg));
  tft.fillRect(104, 206, 96, 32, C_BG);
  tft.setFont(F_S);
  txt(&tft, t2, 152, 222, 1, C_DIM);
  button(B_PSTART, 204, 206, 112, 32, "START", p.nSteps ? C_GREEN : C_BTN, C_WHITE, F_M);
}

// =====================================================================
//  EKRAN: chemia przed startem - swieza czy uzywana (ile rolek)
// =====================================================================
// pierwszy krok danego typu (wywolywacz / blix) - do podgladu doliczanego czasu
int firstStepOfType(int idx, KitStepType t) {
  const Program &p = progs[idx];
  for (int i = 0; i < p.nSteps; i++) if (kitStepType(p.s[i]) == t) return i;
  return -1;
}

void drawChemInfo() {
  auto g = boxBegin(312, 46, C_BG);
  if (!g) return;
  const ChemState &c = chem[curProg];
  const ChemKit &k = KITS[c.kit];
  char b[64], t1[8], t2[8];
  int y = 12;
  for (int pass = 0; pass < 2; pass++) {
    KitStepType st = pass == 0 ? KS_DEV : KS_BX;
    int si = firstStepOfType(curProg, st);
    if (si < 0) continue;
    const Step &s = progs[curProg].s[si];
    float base = stepBaseSec(progs[curProg], s);
    float now = base * kitFactor(c.kit, st, c.used, NAN);
    int add = (int)lroundf(now - base);
    fmtTime(t1, sizeof(t1), (int)lroundf(now));
    snprintf(t2, sizeof(t2), "%d:%02d", add / 60, add % 60);
    if (add > 0) snprintf(b, sizeof(b), "%-5s %s  (+%s)", STEP_SHORT[s.name], t1, t2);
    else snprintf(b, sizeof(b), "%-5s %s", STEP_SHORT[s.name], t1);
    g->setFont(F_MONO);
    txtP(g, b, 4, y, 0, add > 0 ? C_ACC : C_TXT);
    y += 20;
  }
  boxEnd(4, 150);
}

// powyzej wydajnosci podanej przez producenta - tylko ostrzezenie, bez blokady


void drawChemValues() {
  const ChemState &c = chem[curProg];
  const ChemKit &k = KITS[c.kit];
  char b[24];
  auto g = boxBegin(74, 30, C_BG);
  if (g) {
    if (k.maxRolls) snprintf(b, sizeof(b), "%d / %d", c.used, k.maxRolls);
    else snprintf(b, sizeof(b), "%d", c.used);
    g->setFont(F_SB);
    txtP(g, b, chemOverMax(c) ? 31 : 37, 15, 1, c.used > 0 ? C_TXT : C_DIM);
    if (chemOverMax(c)) warnBadge(g, 66, 15);
    boxEnd(194, 86);
  }
  g = boxBegin(212, 30, C_BG);
  if (g) {
    g->setFont(F_SB);
    txtP(g, kitName(c.kit), 106, 15, 1, C_TXT);
    boxEnd(54, 120);
  }
  drawChemInfo();
}

void drawChem() {
  const Program &p = progs[curProg];
  const ChemState &c = chem[curProg];
  char num[32];
  progBadge(curProg, num, sizeof(num));
  hdrAccent = CAT_COL[p.cat];
  drawHeader(TR("CHEMIA", "CHEMISTRY"), true, num);
  bool used = c.used > 0;
  button(B_CFRESH, 6, 38, 151, 40, TR("SWIEZA", "FRESH"), used ? C_BTN2 : C_GREEN, C_WHITE, F_M);
  button(B_CUSED, 163, 38, 151, 40, TR("UZYWANA", "USED"), used ? C_GREEN : C_BTN2, C_WHITE, F_M);
  tft.setFont(F_S);
  txt(&tft, TR("Zuzyte rolki", "Rolls used"), 8, 101, 0, C_DIM);
  button(B_CRM, 150, 86, 40, 30, "-", C_BTN2, C_WHITE, F_M, true);
  button(B_CRP, 272, 86, 42, 30, "+", C_BTN2, C_WHITE, F_M, true);

  button(B_CKM, 6, 120, 44, 30, "<", C_BTN2, C_WHITE, F_M);
  button(B_CKP, 270, 120, 44, 30, ">", C_BTN2, C_WHITE, F_M);
  button(B_CCANCEL, 6, 200, 100, 36, TR("ANULUJ", "CANCEL"), C_BTN2);
  button(B_CGO, 112, 200, 202, 36, "START", C_GREEN, C_WHITE, F_M);
  drawChemValues();
}

void runStart(int prog);
void chemButton(uint16_t id) {
  ChemState &c = chem[curProg];
  int mul = repeatCount > 8 ? 4 : 1;
  switch (id) {
    case B_CFRESH: c.used = 0; fullRedraw = true; break;
    case B_CUSED: if (c.used == 0) c.used = max(1, (int)c.tank); fullRedraw = true; break;
    case B_CRM: c.used = max(0, c.used - mul); if (c.used == 0) fullRedraw = true; break;
    case B_CRP: { bool was0 = c.used == 0; c.used = min(999, c.used + mul); if (was0) fullRedraw = true; break; }
    case B_CKM: c.kit = (c.kit + NUM_KITS - 1) % NUM_KITS; break;
    case B_CKP: c.kit = (c.kit + 1) % NUM_KITS; break;
    case B_CCANCEL: saveChem(curProg); goScreen(S_PROG); return;
    case B_CGO: saveChem(curProg); runStart(curProg); return;
  }
  if (!fullRedraw) drawChemValues();
}

// =====================================================================
//  EKRAN: menu ogolne (ustawienia, podmenu, edytor kroku)
// =====================================================================
enum ItType : uint8_t { IT_INT, IT_TIME, IT_TEMP, IT_TEMPD, IT_BOOL, IT_BOOLN, IT_ENUM, IT_CTRL, IT_ACT };
struct Item {
  const char *label;
  ItType t;
  int16_t *v;
  int16_t mn, mx, st;
  const char *unit;
  const char *const *names;
  uint8_t act;
};
enum : uint8_t { A_NONE, A_SUB_ROLL, A_SUB_STICK, A_SUB_STAND, A_SUB_BWRED, A_SUB_RESET, A_CALIB,
                 A_MOTOR, A_OTA, A_INFO, A_RST_DEF, A_RST_CUST, A_RST_SET, A_FACTORY,
                 A_STEP_UP, A_STEP_DEL };
Item items[16];
uint8_t nItems = 0;
char stepTimeLabel[24];
static const char *const UNIT_NAMES[] = {"~C", "~F"};
#define MROWS 4

void addItem(const char *l, ItType t, int16_t *v, int16_t mn = 0, int16_t mx = 0, int16_t st = 1,
             const char *unit = "", const char *const *names = nullptr, uint8_t act = A_NONE) {
  if (nItems < 16) items[nItems++] = Item{l, t, v, mn, mx, st, unit, names, act};
}

void buildMenu() {
  nItems = 0;
  switch (menuId) {
    case M_SETTINGS:
      addItem(TR("Jezyk", "Language"), IT_ENUM, &gLang, 0, 1, 1, "", LANG_NAMES);
      addItem(TR("Dzwiek", "Sound"), IT_BOOL, &S.sound);
      addItem(TR("Pikanie co minute", "Beep every minute"), IT_BOOLN, &S.noMinuteBeep);
      addItem(TR("Jasnosc", "Brightness"), IT_INT, &S.brightness, 10, 100, 10, "%");
      addItem(TR("Obrot ekranu 180~", "Rotate screen 180~"), IT_BOOL, &S.flip);
      addItem(TR("Jednostki", "Units"), IT_ENUM, &S.tempF, 0, 1, 1, "", UNIT_NAMES);
      addItem(TR("Agitacja ROLL", "Roll agitation"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_SUB_ROLL);
      addItem(TR("Agitacja STICK", "Stick agitation"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_SUB_STICK);
      addItem(TR("Agitacja STAND", "Stand agitation"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_SUB_STAND);
      addItem(TR("Redukcja czasu B&W", "B&W time reduction"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_SUB_BWRED);
      addItem(TR("Prad silnika", "Motor current"), IT_INT, &S.currentmA, CURRENT_MIN_MA, CURRENT_MAX_MA, 50, "mA");
      addItem(TR("Korekta czujnika", "Sensor offset"), IT_TEMPD, &S.tempOffset10, -50, 50, 1);
      addItem(TR("Kalibracja dotyku", "Touch calibration"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_CALIB);
      addItem(TR("Test silnika", "Motor test"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_MOTOR);
      addItem(TR("Aktualizacja WiFi", "WiFi update"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_OTA);
      addItem("Reset", IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_SUB_RESET);
      addItem(TR("Informacje", "Information"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_INFO);
      break;
    case M_ROLL:
      addItem(TR("Predkosc", "Speed"), IT_INT, &S.rollRpm, RPM_MIN, RPM_MAX, 5, "rpm");
      addItem(TR("T1 w prawo", "T1 clockwise"), IT_INT, &S.rollT1, 1, 120, 1, "s");
      addItem(TR("T2 w lewo", "T2 counter-cw"), IT_INT, &S.rollT2, 0, 120, 1, "s");
      // pionowy koreks: zawirowania od zmian kierunku mieszaja ciecz tez w pionie
      addItem(TR("Start intensywny", "Intense start"), IT_INT, &rollStartSec, 0, 120, 5, "s");
      addItem(TR("Burst co (0=wyl)", "Burst every (0=off)"), IT_INT, &rollBurstSec, 0, 120, 5, "s");
      addItem(TR("Zmian w burscie", "Burst reversals"), IT_INT, &rollBurstN, 1, 8, 1, "x");
      break;
    case M_STICK:
      addItem(TR("Predkosc", "Speed"), IT_INT, &S.stickRpm, RPM_MIN, RPM_MAX, 5, "rpm");
      addItem(TR("T1 ciagle", "T1 continuous"), IT_INT, &S.stickT1, 0, 600, 5, "s");
      addItem(TR("T2 co minute", "T2 every min"), IT_INT, &S.stickT2, 0, 59, 1, "s");
      break;
    case M_STAND:
      addItem(TR("Predkosc", "Speed"), IT_INT, &S.standRpm, RPM_MIN, RPM_MAX, 5, "rpm");
      addItem(TR("T1 postoj", "T1 rest"), IT_INT, &S.standT1, 1, 120, 1, "min");
      addItem(TR("T2 na starcie", "T2 at start"), IT_INT, &S.standT2, 0, 600, 5, "s");
      addItem(TR("T3 co cykl", "T3 each cycle"), IT_INT, &S.standT3, 0, 300, 5, "s");
      break;
    case M_BWRED:
      addItem(TR("Redukuj DEV B&W", "Reduce B&W DEV"), IT_BOOL, &S.bwRed);
      addItem(TR("Wielkosc", "Amount"), IT_INT, &S.bwRedPct, 0, 50, 1, "%");
      break;
    case M_RESET:
      addItem("Programy fabr.", IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_RST_DEF);
      addItem(TR("Programy wlasne", "Custom programs"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_RST_CUST);
      addItem("Ustawienia", IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_RST_SET);
      addItem(TR("Reset fabryczny", "Factory reset"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_FACTORY);
      break;
    case M_STEP: {
      Step &s = progs[curProg].s[curStep];
      addItem(TR("Nazwa", "Name"), IT_ENUM, &s.name, 0, NUM_STEP_NAMES - 1, 1, "", STEP_SHORT);
      if (ctrlIsCurve(s.ctrl)) {
        char tb[12];
        snprintf(tb, sizeof(tb), "%.1f~", toUnit(ctrlCurve(s.ctrl).nom10 / 10.0f));
        snprintf(stepTimeLabel, sizeof(stepTimeLabel), TR("Czas @%s", "Time @%s"), tb);
      } else strcpy(stepTimeLabel, TR("Czas", "Time"));
      addItem(stepTimeLabel, IT_TIME, &s.timeSec, 5, 5999);
      addItem(TR("Zmiany wody", "Water changes"), IT_INT, &rinseChg[curProg][curStep], 0, 12, 1, "x");
      addItem(TR("Agitacja", "Agitation"), IT_ENUM, &s.agit, 0, 3, 1, "", AGIT_NAMES);
      addItem(TR("Kontrola czasu", "Time control"), IT_CTRL, &s.ctrl, 0, NUM_CTRL - 1);
      if (s.ctrl == CTRL_MON) {
        addItem("MON min", IT_TEMP, &s.monMin10, 0, 600, 5);
        addItem("MON max", IT_TEMP, &s.monMax10, 0, 600, 5);
      }
      if (curStep > 0) addItem(TR("Przesun w gore", "Move up"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_STEP_UP);
      addItem(TR("Usun krok", "Delete step"), IT_ACT, nullptr, 0, 0, 0, "", nullptr, A_STEP_DEL);
      break;
    }
    case M_TSRC:
      // WL = sonda zanurzona w chemii (pomiar), WYL = sonda w powietrzu (model stygniecia)
      addItem(TR("Czujnik w chemii", "Probe in chemistry"), IT_BOOLN, &runNoTherm);
      addItem(TR("Ilosc chemii", "Chemistry volume"), IT_INT, &runVolMl, 100, 2000, 50, "ml");
      break;
  }
}

const char *menuTitle() {
  switch (menuId) {
    case M_SETTINGS: return TR("USTAWIENIA", "SETTINGS");
    case M_ROLL: return TR("AGITACJA ROLL", "ROLL AGITATION");
    case M_STICK: return TR("AGITACJA STICK", "STICK AGITATION");
    case M_STAND: return TR("AGITACJA STAND", "STAND AGITATION");
    case M_BWRED: return TR("REDUKCJA B&W", "B&W REDUCTION");
    case M_RESET: return "RESET";
    case M_STEP: return TR("EDYCJA KROKU", "EDIT STEP");
    case M_TSRC: return TR("CZUJNIK TEMP.", "TEMP. SENSOR");
  }
  return "";
}

void fmtItem(const Item &it, char *b, size_t n) {
  int v = it.v ? *it.v : 0;
  switch (it.t) {
    case IT_INT:
      if (!strcmp(it.unit, "x")) { if (v == 0) snprintf(b, n, "%s", TR("ciagle", "cont.")); else snprintf(b, n, "%dx", v); }
      else snprintf(b, n, "%d%s", v, strlen(it.unit) <= 1 ? it.unit : "");   // dlugie jednostki przy etykiecie
      break;
    case IT_TIME:  fmtTime(b, n, v); break;
    case IT_TEMP:  snprintf(b, n, "%.1f~", toUnit(v / 10.0f)); break;
    case IT_TEMPD: snprintf(b, n, "%+.1f~", v / 10.0f * (S.tempF ? 1.8f : 1.0f)); break;
    case IT_BOOL:  snprintf(b, n, "%s", v ? TR("WL", "ON") : TR("WYL", "OFF")); break;
    case IT_BOOLN: snprintf(b, n, "%s", v ? TR("WYL", "OFF") : TR("WL", "ON")); break;
    case IT_ENUM:  snprintf(b, n, "%s", it.names[constrain(v, (int)it.mn, (int)it.mx)]); break;
    case IT_CTRL:  snprintf(b, n, "%s", ctrlName(v)); break;
    default: b[0] = 0;
  }
}

void drawMenuRow(int r) {
  int k = menuPage * MROWS + r;
  int y = 36 + r * 50;                               // 4 wysokie wiersze
  bool scroll = nItems > MROWS;
  int right = scroll ? 280 : 316;
  tft.fillRect(0, y, right + 2, 49, C_BG);
  if (r < MROWS - 1) tft.drawFastHLine(6, y + 49, right - 6, C_BORDER);
  for (int j = 0; j < 4; j++) removeBtn(B_ITEM0 + r * 4 + j);
  if (k >= nItems) return;
  const Item &it = items[k];
  tft.setFont(F_S);
  int labMax;                                 // miejsce na etykiete do kontrolek po prawej
  if (it.t == IT_BOOL || it.t == IT_BOOLN) labMax = right - 80 - 12;
  else if (it.t == IT_ACT) labMax = right - 48 - 12 - ((it.act == A_RST_DEF || it.act == A_RST_CUST || it.act == A_RST_SET || it.act == A_FACTORY || it.act == A_STEP_DEL) ? 60 : 0);
  else labMax = right - 132 - 12 - ((it.t == IT_INT && strlen(it.unit) > 1) ? 30 : 0);
  txtFit(it.label, 6, y + 24, 0, C_TXT, labMax);
  if (it.t == IT_INT && strlen(it.unit) > 1) {             // jednostka (mA, rpm, min) obok etykiety
    int lw = min(labMax, (int)tft.textWidth(it.label));
    tft.setFont(&lgfx::fonts::Font0);
    txt(&tft, it.unit, 10 + lw, y + 26, 0, C_DIM);
    tft.setFont(F_S);
  }
  int cx = right - 132;                       // poczatek strefy +/- (38 + 56 + 38)
  uint16_t id = B_ITEM0 + r * 4;
  char b[24];
  switch (it.t) {
    case IT_BOOL:
    case IT_BOOLN: {
      bool on = (it.t == IT_BOOLN) ? !*it.v : *it.v;
      button(id + 2, right - 80, y + 9, 80, 31, on ? TR("WL", "ON") : TR("WYL", "OFF"), on ? C_GREEN : C_BTN2);
      break;
    }
    case IT_ACT: {
      bool danger = it.act == A_RST_DEF || it.act == A_RST_CUST || it.act == A_RST_SET ||
                    it.act == A_FACTORY || it.act == A_STEP_DEL;
      button(id + 2, right - (danger ? 108 : 48), y + 9, danger ? 108 : 48, 31, danger ? TR("WYKONAJ", "EXECUTE") : ">", danger ? C_RED : C_BTN2);
      break;
    }
    default: {
      bool en = it.t == IT_ENUM || it.t == IT_CTRL;
      button(id + 0, cx, y + 4, 38, 41, en ? "<" : "-", C_BTN2, C_WHITE, F_M, !en);
      button(id + 1, cx + 94, y + 4, 38, 41, en ? ">" : "+", C_BTN2, C_WHITE, F_M, !en);
      fmtItem(it, b, sizeof(b));
      tft.fillRect(cx + 38, y + 4, 56, 41, C_BG);
      tft.setFont(F_SB);
      if (strlen(b) > 4) tft.setFont(F_S);
      txt(&tft, b, cx + 66, y + 24, 1, C_TXT);
    }
  }
}

void drawMenu() {
  buildMenu();
  int pages = (nItems + MROWS - 1) / MROWS;
  menuPage = constrain(menuPage, 0, max(0, pages - 1));
  if (menuId == M_STEP) {
    char t[32];
    snprintf(t, sizeof(t), TR("KROK %d/%d", "STEP %d/%d"), curStep + 1, progs[curProg].nSteps);
    drawHeader(t, true);
  } else drawHeader(menuTitle(), true);
  for (int r = 0; r < MROWS; r++) drawMenuRow(r);
  if (nItems > MROWS) drawScrollArrows(B_MUP, B_MDN, 36, 236, menuPage > 0, menuPage < pages - 1,
                                       menuPage + 1, pages);
}

// skutki zmiany wartosci
void applySettingSideEffects(int16_t *v) {
  if (v == &S.brightness) backlightSet(S.brightness);
  else if (v == &S.flip) { lcd.setRotation(S.flip ? 3 : 1); fullRedraw = true; touchLock(400); }
  else if (v == &S.currentmA) { motor.currentmA = S.currentmA; motor.reconfigure(); }
  else if (v == &S.tempOffset10) thermo.offset = S.tempOffset10 / 10.0f;
  else if (v == &S.sound) bootio.soundOn = S.sound;
  else if (v == &S.tempF) fullRedraw = true;
  else if (v == &gLang) { prefs.putShort("lang", gLang); fullRedraw = true; }
}

void menuChange(int r, int dir) {
  int k = menuPage * MROWS + r;
  if (k >= nItems) return;
  Item &it = items[k];
  int v = *it.v;
  if (it.t == IT_ENUM || it.t == IT_CTRL) {
    v += dir;
    if (v > it.mx) v = it.mn;
    if (v < it.mn) v = it.mx;
  } else if (it.t == IT_BOOL || it.t == IT_BOOLN) {
    v = !v;
  } else {
    int mul = repeatCount > 25 ? 10 : (repeatCount > 10 ? 4 : 1);
    int st = it.st;
    if (it.t == IT_TIME) st = v < 120 ? 5 : (v < 600 ? 15 : 30);
    v = constrain(v + dir * st * mul, (int)it.mn, (int)it.mx);
  }
  if (v == *it.v) return;
  *it.v = (int16_t)v;
  if (menuId == M_STEP) {
    progDirty = true;
    if (it.v == &progs[curProg].s[curStep].ctrl || it.v == &progs[curProg].s[curStep].name) {
      Step &s = progs[curProg].s[curStep];
      // wybor MON: ustaw sensowny zakres wokol temperatury programu
      if (it.v == &s.ctrl && s.ctrl == CTRL_MON && s.monMin10 == 0 && s.monMax10 == 0) {
        s.monMin10 = progs[curProg].cat == CAT_BW ? 180 : 300;
        s.monMax10 = progs[curProg].cat == CAT_BW ? 240 : 400;
      }
      fullRedraw = true;          // zmienia sie liczba wierszy / etykiety
      return;
    }
  } else {
    settingsDirty = true;
    applySettingSideEffects(it.v);
  }
  if (!fullRedraw) drawMenuRow(r);
}

// =====================================================================
//  PRZEBIEG PROCESU
// =====================================================================
enum RunPhase : uint8_t { R_POUR, R_RUN, R_OVERRUN, R_DONE };
struct RunState {
  int prog = 0, step = 0;
  RunPhase phase = R_POUR;
  bool paused = false;
  StepTimer timer;
  uint32_t lastTick = 0, overrunStart = 0, procStart = 0;
  uint16_t armedId = 0;
  uint32_t armedAt = 0;
  int lastMinFloor = 0;     // ostatnio ogloszona liczba pelnych minut
  int tankRolls = 1;        // ile rolek bylo w koreksie (do licznika zuzycia)
  int change = 0;           // ktora zmiana wody trwa (plukanie zmianami)
  float Tamb = 22.0f;       // temperatura pokoju na starcie kroku (model stygniecia)
  float Tmodel = 38.0f;     // temperatura chemii wg modelu stygniecia
  float Twall = 21.0f;      // temperatura koreksu (scianki + spirale) wg modelu
  float Troom = 22.0f;      // temperatura pokoju zapisana RAZ na starcie (czujnik w powietrzu)
  bool roomSet = false;
  bool modelOn = false;     // model sledzi koreks od DALEJ na ekranie START
} run;

// Czujnik nie siega chemii (np. Paterson na stojaco) -> czujnik mierzy powietrze,
// a temperature chemii liczy model stygniecia (compensation.h). Pamietane per program.
int16_t runNoTherm = 0;     // 1 = czujnik NIE w chemii
int16_t runVolMl = 500;     // ilosc chemii w koreksie [ml]
int16_t runHoriz = 0;       // 1 = procesor polozony poziomo (koreks lezy)
int16_t runPreT = 45;       // temperatura wody kapieli wstepnej / plukania z kranu [C]
void runTsrcLoad(int prog) {
  char k[8];
  snprintf(k, sizeof(k), "nt%d", progSlot(prog)); runNoTherm = prefs.getShort(k, 0);
  snprintf(k, sizeof(k), "ml%d", progSlot(prog)); runVolMl = constrain((int)prefs.getShort(k, 500), 100, 2000);
  snprintf(k, sizeof(k), "or%d", progSlot(prog)); runHoriz = prefs.getShort(k, 0);
  snprintf(k, sizeof(k), "pw%d", progSlot(prog)); runPreT = constrain((int)prefs.getShort(k, 45), 15, 60);
}
void runTsrcSave() {
  char k[8];
  snprintf(k, sizeof(k), "nt%d", progSlot(run.prog)); prefs.putShort(k, runNoTherm);
  snprintf(k, sizeof(k), "ml%d", progSlot(run.prog)); prefs.putShort(k, runVolMl);
  snprintf(k, sizeof(k), "or%d", progSlot(run.prog)); prefs.putShort(k, runHoriz);
  snprintf(k, sizeof(k), "pw%d", progSlot(run.prog)); prefs.putShort(k, runPreT);
}
float roomTemp() { return run.roomSet ? run.Troom : (thermo.valid ? thermo.t : 22.0f); }
// zapis temperatury pokoju (raz, gdy czujnik wisi w powietrzu)
void runCaptureRoom() {
  if (!run.roomSet && thermo.valid) { run.Troom = thermo.t; run.roomSet = true; }
}

// ---------------------------------------------------------------------
//  Czujnik jako TERMOMETR WODY (gdy nie siega chemii): w kroku z woda
//  (kapiel wstepna, plukanie) woda z kranu raz dotyka sondy, raz nie.
//  Odchylenie od temperatury pokoju > 2 C = kontakt z woda.
//  Sonda ma bezwladnosc: przy krotkim chlapnieciu pokaze mniej niz woda ma
//  naprawde; przy pelnym koreksie odczyt jest temperatura wody.
// ---------------------------------------------------------------------
// Stany (koreks pod kranem, zwykle pelny; wode mozna wylewac i nalewac ponownie):
//  - PELNY:   sonda w wodzie -> temperatura wody = biezacy odczyt. Powolne zmiany
//             (stygniecie, regulacja kranu) nie oznaczaja wylania.
//  - WYLANA:  spadek o co najmniej DRAIN_DROP C w ciagu DRAIN_WIN ms, gdy sonda byla
//             w wodzie (wylewanie trwa 6-7 s, sonda reaguje z opoznieniem). Koreks
//             pusty od POCZATKU spadku: ostatnia chwila, gdy odczyt stal na swoim
//             poziomie (dec*) - model liczy stygniecie od tej chwili, nie od DALEJ.
//  - NALANA:  po wylaniu wzrost o REFILL_RISE C od najnizszego odczytu = caly koreks
//             znow pelny wody o temperaturze z biezacego odczytu.
//  - CHLODNIEJSZA WODA (kran przestawiony z 46 na 43 C tez daje spadek):
//             po "wylaniu" odczyt przestaje spadac (< FLAT_DELTA w FLAT_WIN), a sonda
//             ma wciaz > room + 5 C -> to woda, nie powietrze: koreks pelny,
//             stygniecie policzone od "wylania" jest cofane. Mokra sonda w powietrzu
//             dalej spada w strone temperatury pokoju.
//  - ZABEZPIECZENIA: odczyt wrocil w okolice pokoju (brak kontaktu) -> pusty od
//             poczatku spadku; DALEJ, gdy odczyt juz opadal -> to samo (runNextStep).
// Odczyty DS18B20 (12 bit) przychodza co ok. 0,8 s; historia 20 probek = ok. 14 s.
// Progi sprawdzone symulacja: sonda w wodzie tau ~3 s, w powietrzu 10-90 s.
#define DRAIN_DROP  1.0f
#define DRAIN_WIN   8000
#define REFILL_RISE 0.2f
#define FLAT_DELTA  0.15f
#define FLAT_WIN    4000
#define WT_HIST     20
struct WaterSample { uint32_t t; float T, wall; };
struct WaterTrack {
  WaterSample hist[WT_HIST];  // ostatnie probki (czas, odczyt, temperatura koreksu z modelu)
  uint8_t nHist = 0;
  float lo = 199;             // najnizszy odczyt od wylania
  float last = NAN;           // temperatura wody tuz przed ostatnim wylaniem
  float maxT = NAN;           // najwyzsza temperatura w tym kroku
  float stableRef = NAN;
  uint32_t stableSince = 0;
  uint32_t drainAt = 0;       // poczatek wylewania
  float wallAtDrain = NAN;    // temperatura koreksu w tej chwili
  uint32_t detAt = 0;         // kiedy wykryto wylanie
  float decT = -99;           // poczatek biezacego spadku: odczyt, czas, koreks
  uint32_t decAt = 0;
  float decWall = NAN;
  bool everFull = false;      // byla juz woda w tym kroku
  bool falling = false;       // koreks pusty (wylany)
  bool justDrained = false;   // do modelu: przelicz koreks od chwili wylania
  bool undoDrain = false;     // do modelu: to nie bylo wylanie - przywroc koreks
  bool contact = false, stable = false;
} wt;
void waterReset() { wt = WaterTrack(); }
static void waterDrain(float Tlast, uint32_t at, float wall, uint32_t now, float T) {
  wt.falling = true; wt.justDrained = true;
  wt.last = Tlast; wt.drainAt = at; wt.wallAtDrain = wall;
  wt.detAt = now; wt.lo = T;
}
void waterUpdate() {
  if (!thermo.valid) return;
  float T = thermo.t, Ta = roomTemp();
  uint32_t now = millis();
  // nowa probka co ok. 0,7 s (tempo odczytow DS18B20)
  if (!wt.nHist || now - wt.hist[wt.nHist - 1].t >= 700) {
    if (wt.nHist == WT_HIST) { memmove(wt.hist, wt.hist + 1, sizeof(WaterSample) * (WT_HIST - 1)); wt.nHist = WT_HIST - 1; }
    wt.hist[wt.nHist++] = {now, T, run.Twall};
  }
  wt.contact = fabsf(T - Ta) > 2.0f;
  if (wt.contact) wt.maxT = isnan(wt.maxT) ? T : max(wt.maxT, T);
  if (!wt.falling) {
    if (wt.contact) wt.everFull = true;
    // poczatek biezacego spadku: ostatnia chwila, gdy odczyt nie byl ponizej poziomu
    if (T >= wt.decT - 0.1f) { wt.decT = T; wt.decAt = now; wt.decWall = run.Twall; }
    // spadek o DRAIN_DROP wzgledem maksimum z okna DRAIN_WIN = wylewanie
    int iMax = -1;
    for (int i = 0; i < wt.nHist; i++)
      if (now - wt.hist[i].t <= DRAIN_WIN && (iMax < 0 || wt.hist[i].T > wt.hist[iMax].T)) iMax = i;
    if (wt.everFull && iMax >= 0 && wt.hist[iMax].T - T >= DRAIN_DROP && wt.hist[iMax].T > Ta + 2.0f)
      waterDrain(wt.decT, wt.decAt, wt.decWall, now, T);   // pusty od poczatku spadku
    else if (wt.everFull && !wt.contact)            // sonda wrocila do temperatury pokoju
      waterDrain(wt.decT, wt.decAt, wt.decWall, now, T);
  } else {
    if (T < wt.lo) wt.lo = T;
    int iOld = -1;                                  // probka sprzed ok. FLAT_WIN
    for (int i = 0; i < wt.nHist; i++)
      if (now - wt.hist[i].t >= FLAT_WIN && now - wt.hist[i].t <= FLAT_WIN + 800) iOld = i;
    if (T >= wt.lo + REFILL_RISE && wt.contact) {   // nalana - koreks znow pelny
      wt.falling = false;
      wt.decT = T; wt.decAt = now; wt.decWall = run.Twall;
    } else if (iOld >= 0 && now - wt.detAt >= FLAT_WIN && T > Ta + 5.0f && fabsf(T - wt.hist[iOld].T) < FLAT_DELTA) {
      wt.falling = false; wt.undoDrain = true;      // odczyt sie wyplaszczyl w cieple: chlodniejsza woda
      wt.decT = T; wt.decAt = now; wt.decWall = wt.wallAtDrain;
    }
  }
  if (isnan(wt.stableRef) || fabsf(T - wt.stableRef) > 0.2f) { wt.stableRef = T; wt.stableSince = now; }
  wt.stable = wt.contact && !wt.falling && now - wt.stableSince > 8000;
}
// koreks pelny wody? (do modelu i ekranu)
bool waterFull() { return wt.everFull && !wt.falling && wt.contact; }
// temperatura wody do modelu: pelny = biezacy odczyt, inaczej NAN (koreks pusty)
float waterTemp() { return waterFull() ? thermo.t : NAN; }
bool waterStep(const Step &s) { return !ctrlIsCurve(s.ctrl); }

// ostrzezenia dzwiekowe o temperaturze wody (tylko gdy woda dotyka sondy):
//  za goraca: odczyt > WATER_MAX - pewne mimo bezwladnosci sondy (odczyt zawsze
//             zaniza), pika od razu i co 4 s, dopoki trwa
//  za zimna:  woda tuz przed wylaniem < WATER_MIN albo ustalony odczyt < WATER_MIN
//             (nie przy samym wzroście - sonda dopiero sie nagrzewa), co 6 s
#define WATER_MIN 38.0f
#define WATER_MAX 45.0f
void waterAlarm(bool drainedNow) {
  static uint32_t lastHot = 0, lastCold = 0;
  uint32_t now = millis();
  if (!thermo.valid) return;
  if (wt.contact && thermo.t > WATER_MAX) {
    if (now - lastHot > 4000) { lastHot = now; BEEP(PAT_HOT); }
    return;
  }
  bool cold = (drainedNow && wt.last < WATER_MIN) || (wt.stable && thermo.t < WATER_MIN);
  if (cold && now - lastCold > 6000) { lastCold = now; BEEP(PAT_COLD); }
}
bool stepCools(const Step &s) { return runNoTherm && stepAllowsComp(s) && ctrlIsCurve(s.ctrl); }

// temperatura nominalna procesu (pierwsza krzywa w programie)
float progNominalT(int prog) {
  const Program &p = progs[prog];
  for (int i = 0; i < p.nSteps; i++)
    if (ctrlIsCurve(p.s[i].ctrl)) return ctrlCurve(p.s[i].ctrl).nom10 / 10.0f;
  return 38.0f;
}
// temperatura cieczy wlewanej w kroku j: chemia z krzywa = nominalna,
// kapiel wstepna / plukanie = woda z kranu (ustawienie), MON = srodek zakresu
float runPourTemp(int j) {
  const Step &s = progs[run.prog].s[j];
  if (ctrlIsCurve(s.ctrl)) return ctrlCurve(s.ctrl).nom10 / 10.0f;
  if (s.ctrl == CTRL_MON && s.name != SN_RINSE && s.name != SN_WASH) return (s.monMin10 + s.monMax10) / 20.0f;
  return runPreT;
}
#define WAIT_EST_SEC 90   // szacowane czekanie z pustym koreksem miedzy krokami (1-2 min)

// Temperatura koreksu w chwili wlania kroku i: symulacja od teraz -
// biezacy krok do konca, kolejne kroki z czasami nominalnymi, po kazdym
// WAIT_EST_SEC czekania z pustym koreksem. Dla biezacego kroku (WLEJ)
// zwraca stan z modelu liczonego na zywo (prawdziwy czas czekania).
float runWallAt(int i) {
  float Ta = roomTemp();
  float Tw = run.modelOn ? run.Twall : Ta, Tl = run.Tmodel;
  for (int j = run.step; j < i; j++) {
    const Step &s = progs[run.prog].s[j];
    bool running = j == run.step && run.phase == R_RUN;
    if (j == run.step && run.phase == R_OVERRUN) { Tw = coolEmptyStep(Tw, Ta, runHoriz, WAIT_EST_SEC); continue; }
    float dur = running ? run.timer.remaining() : stepBaseSec(progs[run.prog], s);
    if (!running) Tl = coolPourTemp(runPourTemp(j), Tw, runVolMl);
    coolStep(Tl, Tw, Ta, runVolMl, s.agit, runHoriz, dur);
    Tw = coolEmptyStep(Tw, Ta, runHoriz, WAIT_EST_SEC);
  }
  return Tw;
}
// pierwszy krok z chemia (krzywa) - do podgladu na ekranie START
int firstCurveStep(int prog) {
  for (int i = 0; i < progs[prog].nSteps; i++) if (ctrlIsCurve(progs[prog].s[i].ctrl)) return i;
  return -1;
}
// wlanie kroku: ciecz miesza sie z woda pozostala na sciankach
void runModelPour(const Step &s) {
  if (!run.modelOn) { run.modelOn = true; run.Twall = roomTemp(); }
  run.Tmodel = coolPourTemp(runPourTemp(run.step), run.Twall, runVolMl);
}
// sledzenie w czasie rzeczywistym: z ciecza - model dwuwezlowy, pusty koreks - stygnie
void runModelTick(float dt) {
  const Step &cs = progs[run.prog].s[min(run.step, progs[run.prog].nSteps - 1)];
  if (runNoTherm && run.phase != R_DONE && waterStep(cs)) {   // woda: sonda = termometr wody
    waterUpdate();
    waterAlarm(wt.justDrained);           // wylanie: ostrzez, jesli ostatnia woda byla za zimna
    float Ta = roomTemp();
    if (!run.modelOn) { run.modelOn = true; run.Twall = Ta; }
    if (wt.justDrained) {                 // wylewanie: koreks pusty od POCZATKU szybkiego spadku
      wt.justDrained = false;
      if (!isnan(wt.wallAtDrain)) run.Twall = coolEmptyStep(wt.wallAtDrain, Ta, runHoriz, (millis() - wt.drainAt) / 1000.0f);
    }
    if (wt.undoDrain) {                   // to byla chlodniejsza woda, nie wylanie: cofnij stygniecie
      wt.undoDrain = false;
      if (!isnan(wt.wallAtDrain)) run.Twall = wt.wallAtDrain;
    }
    float w = waterTemp();                // pelny koreks: temperatura wody = biezacy odczyt
    if (!isnan(w)) { run.Tmodel = w; coolStep(run.Tmodel, run.Twall, Ta, runVolMl, cs.agit, runHoriz, dt); }
    else run.Twall = coolEmptyStep(run.Twall, Ta, runHoriz, dt);
    return;
  }
  if (!run.modelOn) return;                 // liczone zawsze w tle (mozna przelaczyc czujnik w trakcie)
  float Ta = roomTemp();
  if (run.phase == R_RUN)
    coolStep(run.Tmodel, run.Twall, Ta, runVolMl, progs[run.prog].s[run.step].agit, runHoriz, dt);
  else if (run.phase == R_OVERRUN || run.phase == R_POUR)
    run.Twall = coolEmptyStep(run.Twall, Ta, runHoriz, dt);
}

// mnoznik zuzycia chemii dla biezacego kroku (zalezy od temperatury - patrz kitFactor)
float runReuseFactor(const Step &s) {
  const ChemState &c = chem[run.prog];
  float T = (thermo.valid && stepAllowsComp(s) && !runNoTherm) ? thermo.t : NAN;
  return kitFactor(c.kit, kitStepType(s), c.used, T);
}

// szacowany czas kroku przy obecnej temperaturze i zuzyciu chemii [s]
float runStepEstimate(int i) {
  const Program &p = progs[run.prog];
  const Step &s = p.s[i];
  float est = stepBaseSec(p, s);
  if (stepCools(s))                     // czujnik w powietrzu: model stygniecia
    return coolStepTime(ctrlCurve(s.ctrl), est, runReuseFactor(s), roomTemp(), runVolMl, s.agit, runHoriz,
                        i == run.step ? runWallAt(i) : progNominalT(run.prog) - 1.0f);
  if (stepAllowsComp(s) && ctrlIsCurve(s.ctrl) && thermo.valid) est *= curveFactor(ctrlCurve(s.ctrl), thermo.t);
  return est * runReuseFactor(s);
}
#define POUR_EST_SEC 10      // wlanie / wylanie (szacunek na krok i na zmiane wody)

// ile zostalo do konca calego procesu [s]: biezacy krok + pozostale kroki,
// kazdy z 15 s ociekania i ~10 s na wlanie
float runRemainingEstimate() {
  const Program &p = progs[run.prog];
  float sum = 0;
  for (int i = run.step; i < p.nSteps; i++) {
    int nch = max(1, (int)rinseChg[run.prog][i]);
    float st = runStepEstimate(i);
    float perChg = st / nch;
    if (i == run.step && run.phase == R_RUN) {
      sum += run.timer.remaining() + (nch - run.change - 1) * (perChg + POUR_EST_SEC) + OVERRUN_SEC;
    } else if (i == run.step && run.phase == R_OVERRUN) {
      sum += max(0, OVERRUN_SEC - (int)((millis() - run.overrunStart) / 1000));
    } else if (i == run.step) {                          // R_POUR (tez kolejna zmiana wody)
      sum += (nch - run.change) * (perChg + POUR_EST_SEC) + OVERRUN_SEC;
    } else {
      sum += st + nch * POUR_EST_SEC + OVERRUN_SEC;
    }
  }
  return sum;
}

void runBeginStep() {
  const Program &p = progs[run.prog];
  const Step &s = p.s[run.step];
  int nch = rinseChg[run.prog][run.step];           // plukanie zmianami: jedna zmiana = czas / liczba zmian
  run.timer.start(stepBaseSec(p, s) / max(1, nch), s.ctrl, stepAllowsComp(s));
  run.Tamb = roomTemp();                            // pokoj w chwili wlania (model stygniecia)
  runModelPour(s);

  run.lastTick = millis();
  run.phase = R_RUN;
  run.paused = false;
  if (s.agit != AG_NONE) agitator.start((AgitMode)s.agit, agitParams());
  else agitator.stop();
  BEEP(PAT_OK);
  fullRedraw = true;
}

void runNextStep() {
  agitator.stop();
  // DALEJ w kroku z woda (czujnik w powietrzu), gdy wylania nie wykryto, ale odczyt
  // juz opadal: woda zostala wylana na poczatku spadku - stygniecie liczymy od tamtej
  // chwili, a nie od DALEJ (wolna sonda w powietrzu nie daje szybkiego spadku)
  if (runNoTherm && run.modelOn && waterStep(progs[run.prog].s[run.step]) && wt.everFull && !wt.falling &&
      thermo.valid && thermo.t < wt.decT - 0.5f && !isnan(wt.decWall))
    run.Twall = coolEmptyStep(wt.decWall, roomTemp(), runHoriz, (millis() - wt.decAt) / 1000.0f);
  waterReset();
  run.step++;
  run.change = 0;
  if (run.step >= progs[run.prog].nSteps) { run.phase = R_DONE; BEEP(PAT_FINISH); }
  else { run.phase = R_POUR; BEEP(PAT_STEP); }
  fullRedraw = true;
}

void runStart(int prog) {
  run = RunState();
  waterReset();
  run.prog = prog;
  runTsrcLoad(prog);
  run.procStart = millis();
  run.lastTick = millis();
  run.tankRolls = max(1, (int)chem[prog].tank);
  motor.currentmA = S.currentmA;
  goScreen(S_SETUP);               // najpierw pytania: ustawienie procesora, czujnik, ilosc chemii
}


// =====================================================================
//  EKRAN: przygotowanie przed startem procesu (pamietane per program)
//   - jak ustawiony jest procesor: PIONOWO / POZIOMO
//   - czy sonda temperatury dotyka chemii: TAK (pomiar) / NIE (model)
//   - ilosc chemii (do modelu stygniecia)
//  DALEJ / BOOT -> ekran "WLEJ: pierwszy krok"
// =====================================================================
void drawSetupValues() {
  char b[16];
  auto g = boxBegin(92, 34, C_BG);
  if (g) {
    snprintf(b, sizeof(b), "%d ml", runVolMl);
    g->setFont(F_SB);
    txtP(g, b, 46, 17, 1, runNoTherm ? C_TXT : C_DIM);
    boxEnd(162, 124);
  }
  g = boxBegin(92, 34, C_BG);
  if (g) {
    snprintf(b, sizeof(b), "%d~C", runPreT);
    g->setFont(F_SB);
    // powyzej ok. 42 C ryzyko dla emulsji (miekniecie, siateczkowanie) - czerwony
    txtP(g, b, 46, 17, 1, runPreT > 42 ? C_RED2 : (runNoTherm ? C_TXT : C_DIM));
    boxEnd(162, 166);
  }
  // podglad: ile bedzie mial koreks przy wlaniu pierwszej chemii (czekanie 1,5 min)
  g = boxBegin(92, 14, C_BG);
  if (g) {
    int fc = firstCurveStep(run.prog);
    if (fc >= 0 && runNoTherm) {
      snprintf(b, sizeof(b), TR("koreks ok. %.0f~", "tank approx. %.0f~"), toUnit(runWallAt(fc)));
      g->setFont(&lgfx::fonts::Font2);
      txtP(g, b, 0, 7, 0, C_ACC);
    }
    boxEnd(8, 187);
  }
}
void drawSetup() {
  const Program &p = progs[run.prog];
  char num[32];
  progBadge(run.prog, num, sizeof(num));
  hdrAccent = CAT_COL[p.cat];
  drawHeader(TR("START", "START"), true, num, false);
  uint16_t lc = runNoTherm ? C_TXT : C_DIM;       // ilosc i koreks licza sie tylko bez czujnika
  tft.setFont(F_S);
  txt(&tft, TR("Procesor", "Processor"), 8, 57, 0, C_TXT);
  button(B_SUV, 100, 38, 106, 38, TR("PIONOWO", "UPRIGHT"), runHoriz ? C_BTN2 : C_GREEN, C_WHITE, F_S);
  button(B_SUH, 210, 38, 106, 38, TR("POZIOMO", "LYING"), runHoriz ? C_GREEN : C_BTN2, C_WHITE, F_S);
  tft.setFont(F_S);
  txt(&tft, TR("Czujnik", "Probe"), 8, 92, 0, C_TXT);
  tft.setFont(&lgfx::fonts::Font2);
  txt(&tft, TR("w chemii?", "in liquid?"), 8, 109, 0, C_DIM);
  button(B_SUPY, 100, 80, 106, 38, TR("TAK", "YES"), runNoTherm ? C_BTN2 : C_GREEN);
  button(B_SUPN, 210, 80, 106, 38, TR("NIE", "NO"), runNoTherm ? C_GREEN : C_BTN2);
  tft.setFont(F_S);
  txt(&tft, TR("Ilosc", "Volume"), 8, 134, 0, lc);
  tft.setFont(&lgfx::fonts::Font2);
  txt(&tft, TR("chemii", "of chem."), 8, 151, 0, C_DIM);
  button(B_SUVM, 100, 122, 44, 38, "-", C_BTN2, C_WHITE, F_M, true);
  button(B_SUVP, 272, 122, 44, 38, "+", C_BTN2, C_WHITE, F_M, true);
  tft.setFont(F_S);
  txtFit(TR("Kapiel wst.", "Pre-wash"), 8, 175, 0, lc, 88);
  button(B_SUTM, 100, 164, 44, 38, "-", C_BTN2, C_WHITE, F_M, true);
  button(B_SUTP, 272, 164, 44, 38, "+", C_BTN2, C_WHITE, F_M, true);
  drawSetupValues();
  button(B_SUGO, 4, 206, 312, 32, TR("DALEJ", "NEXT"), C_GREEN, C_WHITE, F_M);
}
void setupButton(uint16_t id) {
  int mul = repeatCount > 8 ? 4 : 1;
  switch (id) {
    case B_SUV: runHoriz = 0; fullRedraw = true; break;
    case B_SUH: runHoriz = 1; fullRedraw = true; break;
    case B_SUPY: runNoTherm = 0; fullRedraw = true; break;
    case B_SUPN: runNoTherm = 1; fullRedraw = true; break;
    case B_SUVM: runVolMl = max(100, runVolMl - 50 * mul); drawSetupValues(); break;
    case B_SUVP: runVolMl = min(2000, runVolMl + 50 * mul); drawSetupValues(); break;
    case B_SUTM: runPreT = max(15, runPreT - 1); drawSetupValues(); break;
    case B_SUTP: runPreT = min(60, runPreT + 1); drawSetupValues(); break;
    case B_SUGO:
      runTsrcSave();
      if (runNoTherm) runCaptureRoom();       // temperatura pokoju - tylko raz
      if (run.step == 0 && run.phase == R_POUR) run.procStart = millis();
      goScreen(S_RUN);
      break;
  }
}

void runStop() {
  agitator.stop();
  goScreen(S_PROG);
}

void runTick() {
  uint32_t now = millis();
  float dt = (now - run.lastTick) / 1000.0f;
  run.lastTick = now;
  if (dt > 2.0f) dt = 2.0f;
  runModelTick(dt);
  if (run.phase == R_RUN && !run.paused) {
    const Step &cs = progs[run.prog].s[run.step];
    if (stepCools(cs)) {                 // temperatura chemii z modelu stygniecia
      run.timer.tick(dt, run.Tmodel, true, runReuseFactor(cs));
    } else run.timer.tick(dt, thermo.t, thermo.valid, runReuseFactor(cs));
    // Pikanie co pelna minute: przy 3:00 -> 3x, 2:00 -> 2x, 1:00 -> 1x.
    // Kompensacja moze wydluzyc czas (chemia stygnie) - wtedy nie powtarzamy
    // ogloszenia, czekamy az czas zejdzie ponizej kolejnej pelnej minuty.
    int curMin = (int)(run.timer.remaining() / 60.0f);
    if (run.timer.elapsed < 2.0f) run.lastMinFloor = curMin;   // bez ogloszenia tuz po starcie
    else if (curMin < run.lastMinFloor) {
      run.lastMinFloor = curMin;
      if (!S.noMinuteBeep) bootio.play(patMinutes, buildMinutePattern(curMin + 1));
    }
    if (run.timer.done()) {
      int nch = rinseChg[run.prog][run.step];
      if (nch > 0 && run.change + 1 < nch) {          // kolejna zmiana wody: stop, sygnal, czekaj na START
        agitator.stop();
        run.change++;
        run.phase = R_POUR;
        BEEP(PAT_STEP);
        fullRedraw = true;
        return;
      }
      run.phase = R_OVERRUN;               // silnik pracuje jeszcze 15 s
      run.overrunStart = now;
      BEEP(PAT_ALARM);
      fullRedraw = true;
    }
  } else if (run.phase == R_OVERRUN) {
    if (now - run.overrunStart >= OVERRUN_SEC * 1000UL) runNextStep();
  }
  if (run.armedId && now - run.armedAt > 3000) { run.armedId = 0; fullRedraw = true; }
}

// potwierdzenie dwoma dotknieciami (POMIN / STOP)
bool armed(uint16_t id) {
  if (run.armedId == id && millis() - run.armedAt < 3000) { run.armedId = 0; return true; }
  run.armedId = id; run.armedAt = millis(); fullRedraw = true;
  return false;
}

void drawThermometer(lgfx::LovyanGFX *g, int cx, int top, int h, uint16_t col) {
  g->fillRoundRect(cx - 5, top, 10, h, 5, col);
  g->fillCircle(cx, top + h + 4, 10, col);
  g->fillRoundRect(cx - 2, top + h / 3, 4, h, 2, C_RED);
  g->fillCircle(cx, top + h + 4, 7, C_RED);
}

void drawRunDynamic() {
  const Program &p = progs[run.prog];
  if (run.phase == R_DONE) return;
  const Step &s = p.s[run.step];
  char b[24];

  // --- lewy panel: licznik ---
  auto g = boxBegin(212, 128, C_BG);
  if (g) {
    if (run.phase == R_POUR) {
      g->setFont(F_M);
      int nchP = rinseChg[run.prog][run.step];
      if (nchP > 0 && run.change > 0) {               // wymiana wody w trakcie plukania
        g->setFont(F_M);
        txtP(g, TR("ZMIEN WODE", "CHANGE WATER"), 106, 36, 1, C_ACC);
        snprintf(b, sizeof(b), TR("zmiana %d / %d", "change %d / %d"), run.change + 1, nchP);
        g->setFont(F_SB);
        txtP(g, b, 106, 78, 1, C_TXT);
        g->setFont(F_S);
        txtP(g, TR("wylej, wlej, START", "drain, refill, START"), 106, 106, 1, C_TXT2);
        boxEnd(0, 58);
        goto tempPanel;
      }
      txtP(g, TR("WLEJ:", "POUR IN:"), 106, 16, 1, C_DIM);
      g->setFont(F_M);
      txtP(g, stepLong(s.name), 106, 46, 1, C_TXT);
      float est = runStepEstimate(run.step);
      char t[12];
      fmtTime(t, sizeof(t), (int)lroundf(est));
      if (nchP > 0) { char t1[12]; fmtTime(t1, sizeof(t1), (int)lroundf(est / nchP)); snprintf(b, sizeof(b), TR("%d zmian x %s", "%d changes x %s"), nchP, t1); }
      else snprintf(b, sizeof(b), TR("czas ok. %s", "approx. %s"), t);
      g->setFont(F_SB);
      int pw = g->textWidth(b) + 24;                       // "pigulka" z szacowanym czasem
      g->fillRoundRect(106 - pw / 2, 66, pw, 26, 13, C_BTN2);
      txtP(g, b, 106, 79, 1, C_TXT);
      // krok x/y + szacowany czas calego procesu (od tego miejsca do konca)
      int remMin = (int)ceilf(runRemainingEstimate() / 60.0f);
      char ln[48];
      snprintf(ln, sizeof(ln), run.step == 0 ? TR("caly proces ok. %d min", "whole process approx. %d min")
                                             : TR("do konca ok. %d min", "remaining approx. %d min"), remMin);
      g->setFont(F_S);
      txtP(g, ln, 106, 112, 1, C_TXT2);
    } else if (run.phase == R_RUN) {
      int add = (int)lroundf(run.timer.addedSec());
      if ((run.timer.compensate || run.timer.reuseF > 1.001f) && abs(add) >= 1) {
        snprintf(b, sizeof(b), "%c%d:%02d", add >= 0 ? '+' : '-', abs(add) / 60, abs(add) % 60);
        g->setFont(F_SB);
        int pw = g->textWidth(b) + 16;
        g->fillRoundRect(6, 3, pw, 22, 11, rgb565(60, 30, 10));
        txtP(g, b, 6 + pw / 2, 14, 1, C_ACC);
      }
      if (run.paused) { g->setFont(F_SB); txtP(g, TR("PAUZA", "PAUSE"), 204, 14, 2, C_RED2); }
      else if (rinseChg[run.prog][run.step] > 0) {
        snprintf(b, sizeof(b), "%d/%d", run.change + 1, rinseChg[run.prog][run.step]);
        g->setFont(F_SB);
        txtP(g, b, 204, 14, 2, C_ACC);
      }
      fmtTime(b, sizeof(b), (int)ceilf(run.timer.remaining()));
      g->setFont(F_7SEG);
      g->setTextSize(1.4f);
      g->setTextColor(run.paused ? C_DIM : C_TXT);
      g->setTextDatum(lgfx::textdatum_t::middle_center);
      g->drawString(b, 106, 66);
      g->setTextSize(1);
      // gruby zaokraglony pasek postepu + procent
      float pr = constrain(run.timer.progress, 0.0f, 1.0f);
      g->fillRoundRect(8, 110, 154, 10, 5, C_BTN2);
      int fw = max(10, (int)(154 * pr));
      g->fillRoundRect(8, 110, fw, 10, 5, C_ACC);
      g->fillCircle(8 + fw - 5, 115, 3, rgb565(255, 190, 140));     // jasna "glowica"
      snprintf(b, sizeof(b), "%d%%", (int)(pr * 100));
      g->setFont(F_SB);
      txtP(g, b, 206, 115, 2, C_TXT2);
    } else if (run.phase == R_OVERRUN) {
      g->setFont(F_L);
      txtP(g, TR("WYLEJ", "DRAIN"), 8, 30, 0, C_RED2);
      int left = max(0, OVERRUN_SEC - (int)((millis() - run.overrunStart) / 1000));
      // pierscien odliczania 15 s
      float fr = left / (float)OVERRUN_SEC;
      g->fillArc(172, 70, 38, 31, 0, 360, C_BTN2);
      if (fr > 0) g->fillArc(172, 70, 38, 31, -90, -90 + (int)(360 * fr), C_RED);
      snprintf(b, sizeof(b), "%d", left);
      g->setFont(F_L);
      txtP(g, b, 172, 70, 1, C_TXT);
      g->setFont(F_S);
      txtP(g, TR("silnik stop", "motor stop"), 8, 62, 0, C_TXT2);
      if (run.step + 1 < p.nSteps) {
        txtP(g, TR("dalej:", "next:"), 8, 92, 0, C_DIM);
        g->setFont(F_SB);
        txtP(g, stepLong(p.s[run.step + 1].name), 8, 112, 0, CAT_COL[p.cat]);
      }
    }
    boxEnd(0, 58);
  }

tempPanel:
  // --- prawy panel: temperatura (szary = w normie, czerwony = poza) ---
  // Czujnik "w powietrzu" (runNoTherm): duza liczba = temperatura chemii z modelu
  // stygniecia (w trakcie kroku) albo temperatura pokoju (przed startem).
  bool cools = stepCools(s);
  bool inR = thermo.valid && tempInRange(s.ctrl, s.monMin10, s.monMax10, thermo.t);
  uint16_t bg = (s.ctrl == CTRL_OFF || runNoTherm) ? C_TOK : (inR ? C_TOK : C_REJ);
  const uint16_t C_SOFT = rgb565(170, 170, 170);
  g = boxBegin(106, 128, bg);
  if (g) {
    if (runNoTherm && waterStep(s)) {   // krok z woda: sonda jako termometr wody z kranu
      waterUpdate();
      drawThermometer(g, 16, 8, 30, C_WHITE);
      // kolor wg zakresu do plukania: za zimna / dobra 38-45 / za goraca
      auto rangeCol = [](float t) -> uint16_t { return t < WATER_MIN ? CAT_COL[CAT_E6] : (t <= WATER_MAX ? C_OK : C_RED2); };
      bool full = waterFull();
      // duza liczba: biezacy odczyt (przy pelnym koreksie = temperatura wody)
      if (thermo.valid) snprintf(b, sizeof(b), "%.1f", toUnit(thermo.t)); else snprintf(b, sizeof(b), "--.-");
      g->setFont(F_L);
      if (g->textWidth(b) > 76) g->setFont(F_M);
      txtP(g, b, 66, 24, 1, full ? rangeCol(thermo.t) : C_WHITE);
      g->setFont(F_SB);
      if (full) txtP(g, TR("woda", "water"), 66, 50, 1, rangeCol(thermo.t));
      else if (wt.falling) txtP(g, TR("wylana", "drained"), 66, 50, 1, C_ACC);
      else txtP(g, TR("czujnik", "probe"), 66, 50, 1, C_SOFT);
      if (full) {                        // koreks pelny wody o tej temperaturze
        g->setFont(F_SB);
        txtP(g, wt.stable ? TR("ustalona", "steady") : TR("pelny koreks", "tank full"), 53, 76, 1, wt.stable ? C_OK : C_TXT);
        g->setFont(F_S);
        if (!isnan(wt.maxT)) { snprintf(b, sizeof(b), "max %.1f~", toUnit(wt.maxT)); txtP(g, b, 53, 97, 1, C_SOFT); }
      } else if (wt.falling) {           // wylana: ostatnia woda + jak dlugo koreks stoi pusty
        snprintf(b, sizeof(b), TR("byla %.1f~", "was %.1f~"), toUnit(wt.last));
        g->setFont(F_SB);
        txtP(g, b, 53, 76, 1, rangeCol(wt.last));
        int e = (millis() - wt.drainAt) / 1000;
        snprintf(b, sizeof(b), TR("pusty %d:%02d", "empty %d:%02d"), e / 60, e % 60);
        g->setFont(F_S);
        txtP(g, b, 53, 97, 1, C_SOFT);
      } else {
        g->setFont(F_S);
        txtP(g, TR("czekam na wode", "waiting for water"), 53, 78, 1, C_SOFT);
      }
      g->setFont(&lgfx::fonts::Font0);
      snprintf(b, sizeof(b), TR("koreks %.1f~", "tank %.1f~"), toUnit(run.Twall));
      txtP(g, b, 53, 119, 1, C_SOFT);
      boxEnd(214, 58);
      return;
    }
    float Tshow = runNoTherm ? roomTemp() : thermo.t;          // czujnik w powietrzu: pokoj z poczatku
    bool vShow = runNoTherm ? true : (thermo.valid && !isnan(thermo.t));
    float Tst = 0, Tend = 0, ext = 0;
    if (cools && run.phase == R_RUN) { Tshow = run.Tmodel; vShow = true; }
    else if (cools) {                    // przed wlaniem: symulacja calego kroku
      const Curve &cv = ctrlCurve(s.ctrl);
      float base = stepBaseSec(p, s), reuse = runReuseFactor(s);
      float tReal = coolStepTime(cv, base, reuse, roomTemp(), runVolMl, s.agit, runHoriz, runWallAt(run.step), &Tend, &Tst);
      ext = tReal - base * reuse;
    }
    // gora: maly termometr + obok duza temperatura (jednostka pod liczba)
    drawThermometer(g, 16, 8, 30, runNoTherm ? C_SOFT : C_WHITE);
    if (vShow) snprintf(b, sizeof(b), "%.1f", toUnit(Tshow));
    else snprintf(b, sizeof(b), "--.-");
    g->setFont(F_L);
    if (g->textWidth(b) > 76) g->setFont(F_M);
    txtP(g, b, 66, 24, 1, C_WHITE);
    g->setFont(F_SB);
    const char *unitLbl = S.tempF ? "~F" : "~C";
    if (runNoTherm) unitLbl = (run.phase == R_RUN && cools) ? TR("model", "model") : TR("pokoj", "room");
    txtP(g, unitLbl, 66, 50, 1, runNoTherm ? C_ACC : rgb565(200, 200, 200));
    if (run.phase == R_RUN) {
      // laczny mnoznik duzy; pod nim na zmiane co 3 s: temperatura / chemia (/ pokoj)
      snprintf(b, sizeof(b), "x%.2f", run.timer.factor);
      g->setFont(F_M);
      txtP(g, b, 53, 84, 1, C_WHITE);
      int n = runNoTherm ? 3 : 2;
      int ph = (millis() / 3000) % n;
      uint16_t c2 = C_SOFT;
      if (ph == 0) { snprintf(b, sizeof(b), TR("temp. x%.2f", "temp. x%.2f"), run.timer.curveF); if (run.timer.compensate) c2 = C_TXT; }
      else if (ph == 1) { snprintf(b, sizeof(b), TR("chemia x%.2f", "chem. x%.2f"), run.timer.reuseF); if (run.timer.reuseF > 1.001f) c2 = C_ACC; }
      else snprintf(b, sizeof(b), TR("koreks %.1f~", "tank %.1f~"), toUnit(run.Twall));
      g->setFont(F_S);
      txtP(g, b, 53, 113, 1, c2);
    } else {
      if (runNoTherm) {
        // czujnik w powietrzu: temperatura koreksu wg modelu + prognoza kroku
        g->setFont(F_S);
        snprintf(b, sizeof(b), TR("koreks %.1f~", "tank %.1f~"), toUnit(runWallAt(run.step)));
        txtP(g, b, 53, 72, 1, C_ACC);
        if (cools) {                      // temp. po wlaniu -> na koncu, zmiana czasu
          g->setFont(F_S);
          snprintf(b, sizeof(b), "%.1f>%.1f~", toUnit(Tst), toUnit(Tend));
          txtP(g, b, 53, 93, 1, C_TXT);
          int e = (int)lroundf(ext);
          snprintf(b, sizeof(b), "%c%d:%02d", e < 0 ? '-' : '+', abs(e) / 60, abs(e) % 60);
          g->setFont(F_SB);
          txtP(g, b, 53, 114, 1, e > 0 ? C_ACC : C_TXT);
        }
      } else if (ctrlIsCurve(s.ctrl) && stepAllowsComp(s)) {
        // przed startem kroku: temperatura docelowa
        snprintf(b, sizeof(b), TR("cel %d~", "target %d~"), (int)lroundf(toUnit(ctrlCurve(s.ctrl).nom10 / 10.0f)));
        g->setFont(F_SB);
        txtP(g, b, 53, 88, 1, rgb565(200, 200, 200));
      }
      if (!runNoTherm) {                  // przy czujniku w powietrzu brak miejsca - panel i tak reaguje
        g->setFont(&lgfx::fonts::Font0);
        txtP(g, TR("dotknij = opcje", "tap = options"), 53, 119, 1, C_SOFT);
      }
    }
    boxEnd(214, 58);
  }
}

void drawRun() {
  const Program &p = progs[run.prog];
  char num[32], t[32];
  progBadge(run.prog, num, sizeof(num));
  hdrAccent = CAT_COL[p.cat];
  if (run.phase == R_DONE) {
    drawHeader("", false, num, false);
  } else {
    // naglowek: [ADOX C-41] ............ rolka 6/16  (pozniej: krok 2/6)
    drawHeader("", false, num, false);
    const ChemState &c = chem[run.prog];
    // przed startem (pierwszy krok, WLEJ): ktora rolka z ilu - kontrola chemii;
    // pozniej w tym miejscu: ktory krok z ilu
    char r[24];
    bool atStart = run.step == 0 && run.phase == R_POUR;
    int from = c.used + 1, to = c.used + run.tankRolls, mx = KITS[c.kit].maxRolls;
    const char *lbl = TR("rolka", "roll");
    if (!atStart) { snprintf(r, sizeof(r), "%d/%d", run.step + 1, p.nSteps); lbl = TR("krok", "step"); mx = 0; }
    else if (c.kit == KIT_NONE) r[0] = 0;
    else if (to > from && mx) snprintf(r, sizeof(r), "%d-%d/%d", from, to, mx);
    else if (to > from) snprintf(r, sizeof(r), "%d-%d", from, to);
    else if (mx) snprintf(r, sizeof(r), "%d/%d", from, mx);
    else snprintf(r, sizeof(r), "%d", from);
    if (r[0]) {
      bool over = mx && to > mx;
      tft.setFont(F_SB);
      int rx = over ? 298 : 314;
      int rw = tft.textWidth(r);
      txt(&tft, r, rx, HDR_H / 2, 2, over ? C_ACC : C_TXT);
      if (over) warnBadge(&tft, 308, HDR_H / 2);
      tft.setFont(F_S);
      txt(&tft, lbl, rx - rw - 5, HDR_H / 2, 2, C_DIM);
    }
  }

  if (run.phase == R_DONE) {
    tft.setFont(F_M);
    tft.setFont(F_L);
    txt(&tft, TR("GOTOWE!", "DONE!"), 160, 78, 1, C_OK);
    char tt[12], b[48];
    fmtTime(tt, sizeof(tt), (millis() - run.procStart) / 1000);
    snprintf(b, sizeof(b), TR("czas calkowity %s", "total time %s"), tt);
    tft.setFont(F_S);
    txt(&tft, b, 160, 116, 1, C_TXT2);
    // licznik zuzycia chemii: ile rolek bylo w koreksie
    tft.setFont(F_S);
    txt(&tft, TR("Rolek w koreksie", "Rolls in tank"), 8, 162, 0, C_DIM);
    button(B_DTM, 166, 147, 40, 30, "-", C_BTN2, C_WHITE, F_M);
    button(B_DTP, 274, 147, 40, 30, "+", C_BTN2, C_WHITE, F_M);
    snprintf(b, sizeof(b), "%d", run.tankRolls);
    tft.fillRect(208, 147, 64, 30, C_BG);
    tft.setFont(F_M);
    txt(&tft, b, 240, 162, 1, C_TXT);
    const ChemState &c = chem[run.prog];
    if (KITS[c.kit].maxRolls) snprintf(b, sizeof(b), TR("zuzycie chemii: %d -> %d / %d rolek", "chemistry use: %d -> %d / %d rolls"), c.used, c.used + run.tankRolls, KITS[c.kit].maxRolls);
    else snprintf(b, sizeof(b), TR("zuzycie chemii: %d -> %d rolek", "chemistry use: %d -> %d rolls"), c.used, c.used + run.tankRolls);
    tft.fillRect(0, 179, SCR_W, 14, C_BG);
    tft.setFont(&lgfx::fonts::Font0);
    txt(&tft, b, 160, 186, 1, C_DIM);
    button(B_ROK, 60, 198, 200, 38, TR("ZAPISZ I ZAKONCZ", "SAVE & FINISH"), C_GREEN, C_WHITE, F_SB);
    return;
  }

  // pasek informacji o kroku: DEV 03:15 ROLL C41T 38C
  const Step &s = p.s[run.step];
  tft.fillRect(0, HDR_H, SCR_W, 24, C_BAR);
  char b[16];
  tft.setFont(F_MONO);
  txt(&tft, STEP_SHORT[s.name], 8, HDR_H + 12, 0, C_TXT);
  fmtTime(b, sizeof(b), (int)stepNominalSec(run.prog, s));
  txt(&tft, b, 72, HDR_H + 12, 0, C_TXT2);
  txt(&tft, AGIT_NAMES[s.agit], 140, HDR_H + 12, 0, C_TXT2);
  if (ctrlIsCurve(s.ctrl)) snprintf(t, sizeof(t), "%s %d~", ctrlName(s.ctrl), (int)lroundf(toUnit(ctrlCurve(s.ctrl).nom10 / 10.0f)));
  else snprintf(t, sizeof(t), "%s", ctrlName(s.ctrl));
  if (ctrlIsCurve(s.ctrl) && !stepAllowsComp(s)) snprintf(t, sizeof(t), "MON");
  tft.setFont(F_SB);
  txt(&tft, t, 314, HDR_H + 12, 2, CAT_COL[p.cat]);

  tft.fillRect(0, 186, SCR_W, 54, C_BG);
  bool aStop = run.armedId == B_RSTOP, aSkip = run.armedId == B_RSKIP;
  switch (run.phase) {
    case R_POUR:
      button(B_RSTART, 4, 190, 200, 46, "START", C_GREEN, C_WHITE, F_M);
      button(B_RSKIP, 208, 190, 52, 46, aSkip ? "?" : ">>", aSkip ? C_ACC : C_BTN2);
      button(B_RSTOP, 264, 190, 52, 46, aStop ? "?" : "X", aStop ? C_RED2 : C_RED);
      addBtn(214, 58, 106, 128, B_RTEMP);              // panel temperatury -> opcje czujnika
      break;
    case R_RUN:
      button(B_RPAUSE, 4, 190, 120, 46, run.paused ? TR("WZNOW", "RESUME") : TR("PAUZA", "PAUSE"), run.paused ? C_GREEN : C_BTN2, C_WHITE, F_M);
      button(B_RSKIP, 128, 190, 92, 46, aSkip ? TR("PEWNO?", "SURE?") : TR("POMIN", "SKIP"), aSkip ? C_ACC : C_BTN2);
      button(B_RSTOP, 224, 190, 92, 46, aStop ? TR("PEWNO?", "SURE?") : "STOP", aStop ? C_RED2 : C_RED);
      break;
    case R_OVERRUN:
      button(B_RNEXT, 4, 190, 212, 46, TR("DALEJ", "NEXT"), C_GREEN, C_WHITE, F_M);
      button(B_RSTOP, 224, 190, 92, 46, aStop ? TR("PEWNO?", "SURE?") : "STOP", aStop ? C_RED2 : C_RED);
      break;
    default: break;
  }
  drawRunDynamic();
}

// po udanym procesie: dolicz rolki do zuzycia chemii i zapamietaj (pamiec trwala)
void runFinishSave() {
  ChemState &c = chem[run.prog];
  c.used = min(999, c.used + run.tankRolls);
  c.tank = run.tankRolls;
  saveChem(run.prog);
}

void runButton(uint16_t id) {
  switch (id) {
    case B_RSTART: if (run.phase == R_POUR) runBeginStep(); break;
    case B_RPAUSE:
      if (run.phase == R_RUN) { run.paused = !run.paused; agitator.pause(run.paused); fullRedraw = true; }
      break;
    case B_RSKIP: if (armed(B_RSKIP)) runNextStep(); break;
    case B_RSTOP: if (armed(B_RSTOP)) runStop(); break;
    case B_RNEXT: runNextStep(); break;
    case B_DTM: run.tankRolls = max(1, run.tankRolls - 1); fullRedraw = true; break;
    case B_DTP: run.tankRolls = min(8, run.tankRolls + 1); fullRedraw = true; break;
    case B_ROK: runFinishSave(); goScreen(S_PROG); break;
    case B_RTEMP: if (run.phase == R_POUR) goScreen(S_SETUP); break;
  }
}

// przycisk BOOT: START / PAUZA / DALEJ
void bootPressed() {
  if (scr == S_CALIB && calTest) { calibAccept(); return; }
  if (scr == S_SETUP) { setupButton(B_SUGO); return; }
  if (scr != S_RUN) return;
  switch (run.phase) {
    case R_POUR: runBeginStep(); break;
    case R_RUN: runButton(B_RPAUSE); break;
    case R_OVERRUN: runNextStep(); break;
    case R_DONE: runFinishSave(); goScreen(S_PROG); break;
  }
}

// =====================================================================
//  EKRAN: termometr
// =====================================================================
// kolor temperatury: zimna niebieska, pokojowa biala, ciepla pomaranczowa (C-41), goraca czerwona
uint16_t tempColor(float t) {
  if (isnan(t)) return C_DIM;
  if (t < 18) return CAT_COL[CAT_E6];
  if (t < 30) return C_TXT;
  if (t < 41) return C_ACC;
  return C_RED2;
}

void drawThermoDynamic() {
  char b[24];
  auto g = boxBegin(320, 90, C_BG);
  if (g) {
    if (thermo.valid) snprintf(b, sizeof(b), "%.1f", toUnit(thermo.t));
    else snprintf(b, sizeof(b), "--.-");
    g->setFont(F_7SEG);
    g->setTextSize(1.6f);
    g->setTextColor(thermo.valid ? tempColor(thermo.t) : C_DIM);
    g->setTextDatum(lgfx::textdatum_t::middle_right);
    g->drawString(b, 236, 45);
    g->setTextSize(1);
    g->setFont(F_L);
    txtP(g, S.tempF ? "~F" : "~C", 244, 30, 0, C_TXT);
    boxEnd(0, 36);
  }
  g = boxBegin(320, 22, C_BG);
  if (g) {
    char a[12], c[12];
    fmtTemp(a, sizeof(a), thermo.tMin, !isnan(thermo.tMin));
    fmtTemp(c, sizeof(c), thermo.tMax, !isnan(thermo.tMax));
    char m1[20], m2[20];
    snprintf(m1, sizeof(m1), "min %s", a);
    snprintf(m2, sizeof(m2), "max %s", c);
    g->setFont(F_S);
    txtP(g, m1, 150, 11, 2, C_TXT2);
    txtP(g, m2, 170, 11, 0, C_TXT2);
    boxEnd(0, 128);
  }
  // wykres ostatnich 5 minut
  g = boxBegin(300, 52, C_BG);
  if (g) {
    g->drawRect(0, 0, 300, 52, C_BTN2);
    uint16_t n = thermo.histN;
    if (n >= 2) {
      float lo = 1000, hi = -1000;
      for (uint16_t i = 0; i < n; i++) {
        float v = thermo.hist[(thermo.histPos + THERMO_HIST - n + i) % THERMO_HIST];
        lo = min(lo, v); hi = max(hi, v);
      }
      if (hi - lo < 1.0f) { float m = (hi + lo) / 2; lo = m - 0.5f; hi = m + 0.5f; }
      int px = -1, py = -1;
      for (uint16_t i = 0; i < n; i++) {
        float v = thermo.hist[(thermo.histPos + THERMO_HIST - n + i) % THERMO_HIST];
        int x = 2 + (int)((296.0f * i) / (THERMO_HIST - 1));
        int y = 49 - (int)((v - lo) / (hi - lo) * 46);
        if (px >= 0) { g->drawLine(px, py, x, y, C_ACC); g->drawLine(px, py + 1, x, y + 1, C_ACC); }
        px = x; py = y;
      }
    }
    boxEnd(10, 152);
  }
}
void drawThermo() {
  drawHeader(TR("TERMOMETR", "THERMOMETER"), true, nullptr, false);
  if (!thermo.sensorFound()) {
    tft.setFont(F_S);
    txt(&tft, TR("Brak czujnika DS18B20 (GPIO20)", "No DS18B20 sensor (GPIO20)"), 160, 120, 1, C_RED);
  }
  button(B_TRESET, 60, 208, 200, 30, "RESET MIN/MAX", C_BTN2);
  drawThermoDynamic();
}

// =====================================================================
//  EKRAN: test silnika
// =====================================================================
bool motorTestOn = false;
int motorTestDir = 1;
void drawMotorValues() {
  char b[24];
  auto g = boxBegin(100, 32, C_BG);
  if (g) { snprintf(b, sizeof(b), "%d rpm", S.rollRpm); g->setFont(F_SB); txtP(g, b, 50, 16, 1, C_WHITE); boxEnd(110, 44); }
  g = boxBegin(100, 32, C_BG);
  if (g) { snprintf(b, sizeof(b), "%d mA", S.currentmA); g->setFont(F_SB); txtP(g, b, 50, 16, 1, C_WHITE); boxEnd(110, 84); }
}
void drawMotor() {
  drawHeader(TR("TEST SILNIKA", "MOTOR TEST"), true);
  tft.setFont(F_S);
  txt(&tft, TR("Predkosc", "Speed"), 6, 60, 0, C_TXT);
  txt(&tft, TR("Prad", "Current"), 6, 100, 0, C_TXT);
  button(B_MRPMM, 220, 44, 46, 32, "-", C_BTN2, C_WHITE, F_M, true);
  button(B_MRPMP, 270, 44, 46, 32, "+", C_BTN2, C_WHITE, F_M, true);
  button(B_MCURM, 220, 84, 46, 32, "-", C_BTN2, C_WHITE, F_M, true);
  button(B_MCURP, 270, 84, 46, 32, "+", C_BTN2, C_WHITE, F_M, true);
  button(B_MDIR, 6, 128, 150, 40, motorTestDir > 0 ? TR("W PRAWO", "CW") : TR("W LEWO", "CCW"), C_BTN2);
  button(B_MGO, 164, 128, 152, 40, motorTestOn ? "STOP" : "START", motorTestOn ? C_RED : C_GREEN, C_WHITE, F_M);
  tft.setFont(F_S);
  txt(&tft, TR("TMC2209: UART GPIO21 (tylko zapis)", "TMC2209: UART GPIO21 (write only)"), 160, 190, 1, C_DIM);
  txt(&tft, TR("Predkosc zapisywana jako ROLL", "Speed is saved as ROLL"), 160, 212, 1, C_DIM);
  drawMotorValues();
}
void motorButton(uint16_t id) {
  int mul = repeatCount > 10 ? 4 : 1;
  switch (id) {
    case B_MRPMM: S.rollRpm = constrain(S.rollRpm - 1 * mul, RPM_MIN, RPM_MAX); settingsDirty = true; break;
    case B_MRPMP: S.rollRpm = constrain(S.rollRpm + 1 * mul, RPM_MIN, RPM_MAX); settingsDirty = true; break;
    case B_MCURM: S.currentmA = constrain(S.currentmA - 50, CURRENT_MIN_MA, CURRENT_MAX_MA); settingsDirty = true;
                  motor.currentmA = S.currentmA; motor.reconfigure(); break;
    case B_MCURP: S.currentmA = constrain(S.currentmA + 50, CURRENT_MIN_MA, CURRENT_MAX_MA); settingsDirty = true;
                  motor.currentmA = S.currentmA; motor.reconfigure(); break;
    case B_MDIR: motorTestDir = -motorTestDir; fullRedraw = true; break;
    case B_MGO: motorTestOn = !motorTestOn; motor.currentmA = S.currentmA; fullRedraw = true; break;
  }
  if (motorTestOn) motor.setTarget(motorTestDir * S.rollRpm); else motor.setTarget(0);
  if (!fullRedraw) drawMotorValues();
}

// =====================================================================
//  EKRAN: kalibracja dotyku (3 punkty)
//  Zabezpieczenia (jedno dotkniecie nie moze zepsuc kalibracji):
//   - punkt przyjmowany dopiero po przytrzymaniu >= ~150 ms,
//   - po przyjeciu punktu: blokada dotyku 500 ms + wymagane puszczenie,
//   - punkt za blisko poprzedniego (podwojne klikniecie) = odrzucony,
//   - wynik sprawdzany (skala + trafienie w krzyzyki),
//   - nowa kalibracja zapisywana dopiero po GOTOWE (albo BOOT);
//     bez potwierdzenia przez 20 s wraca stara i kalibracja od nowa.
// =====================================================================
static const int16_t CAL_X[3] = {30, 290, 30};
static const int16_t CAL_Y[3] = {30, 30, 210};
#define CAL_MIN_SAMPLES 10        // x15 ms = ~150 ms przytrzymania
#define CAL_CONFIRM_MS  20000
int calIdx = 0;
int16_t calRx[3], calRy[3];
long calSumX = 0, calSumY = 0;
int calN = 0;
bool calTest = false;
TouchCal calOld;                  // kalibracja sprzed rozpoczecia (do cofniecia)
uint32_t calTestAt = 0;
const char *calMsg = nullptr;     // komunikat o odrzuconym punkcie

void drawCross(int x, int y, uint16_t c) {
  tft.drawLine(x - 14, y, x + 14, y, c);
  tft.drawLine(x, y - 14, x, y + 14, c);
  tft.drawCircle(x, y, 7, c);
}
void drawCalib() {
  tft.fillScreen(C_BG);
  tft.setFont(F_SB);
  if (calTest) {
    txt(&tft, TR("Sprawdz kalibracje", "Check calibration"), 160, 62, 1, C_TXT);
    tft.setFont(F_S);
    txt(&tft, TR("Dotykaj ekran - kropki sprawdzaja trafienie", "Touch the screen - dots show accuracy"), 160, 92, 1, C_WHITE);
    txt(&tft, TR("GOTOWE lub BOOT = zapisz", "DONE or BOOT = save"), 160, 118, 1, C_DIM);
    txt(&tft, TR("brak potwierdzenia 20 s = od nowa", "no confirm in 20 s = start over"), 160, 138, 1, C_DIM);
    button(B_CAGAIN, 10, 190, 145, 44, TR("POWTORZ", "REPEAT"), C_BTN2);
    button(B_CDONE, 165, 190, 145, 44, TR("GOTOWE", "DONE"), C_GREEN, C_WHITE, F_M);
    return;
  }
  char b[40];
  snprintf(b, sizeof(b), TR("Dotknij krzyzyk %d/3", "Touch the cross %d/3"), calIdx + 1);
  txt(&tft, b, 160, 110, 1, C_TXT);
  tft.setFont(F_S);
  txt(&tft, TR("dokladnie w srodek, przytrzymaj chwile", "hit the centre and hold briefly"), 160, 136, 1, C_DIM);
  if (calMsg) txt(&tft, calMsg, 160, 158, 1, C_RED2);
  drawCross(CAL_X[calIdx], CAL_Y[calIdx], C_TXT);
}
void calibStart() {
  if (scr != S_CALIB || !calTest) calOld = S.cal;   // przy POWTORZ zostaje pierwotna
  else S.cal = calOld;
  calIdx = 0; calTest = false; calN = 0; calSumX = calSumY = 0; calMsg = nullptr;
  touchLock(400);
  goScreen(S_CALIB);
}

// czy wyliczona kalibracja jest sensowna: skala zblizona do 1 piksel/jednostke
// i wszystkie 3 punkty trafiaja w swoje krzyzyki
static bool calibPlausible(const TouchCal &c) {
  float bx = fabsf(c.bx), by = fabsf(c.by);
  if (bx < 0.4f || bx > 2.5f || by < 0.4f || by > 2.5f) return false;
  for (int i = 0; i < 3; i++) {
    float sx = c.swap ? calRy[i] : calRx[i];
    float sy = c.swap ? calRx[i] : calRy[i];
    float fx = c.ax + c.bx * sx, fy = c.ay + c.by * sy;
    if (S.flip) { fx = 319 - fx; fy = 239 - fy; }
    if (fabsf(fx - CAL_X[i]) > 30 || fabsf(fy - CAL_Y[i]) > 30) return false;
  }
  return true;
}

void calibAccept() {             // GOTOWE / BOOT
  saveSettings();
  calTest = false;
  BEEP(PAT_OK);
  touchLock(300);
  goStartScreen();
}
void calibTick() {               // z loop(): brak potwierdzenia -> cofnij
  if (scr == S_CALIB && calTest && millis() - calTestAt > CAL_CONFIRM_MS) {
    S.cal = calOld;
    calTest = false;
    calibStart();
  }
}

// wywolywane z petli: down = po debounce, rawDown = faktyczny odczyt
void calibTouch(bool down, bool rawDown, int16_t rx, int16_t ry) {
  static bool was = false;
  if (rawDown) { calSumX += rx; calSumY += ry; calN++; }
  if (!down && was) {
    if (calN >= CAL_MIN_SAMPLES) {
      int16_t px = calSumX / calN, py = calSumY / calN;
      bool tooClose = false;
      for (int i = 0; i < calIdx; i++)
        if (abs(px - calRx[i]) + abs(py - calRy[i]) < 60) tooClose = true;
      if (tooClose) {
        calMsg = TR("za blisko poprzedniego - dotknij krzyzyk", "too close to previous - touch the cross");
        BEEP(PAT_ALARM);
      } else {
        calRx[calIdx] = px; calRy[calIdx] = py;
        calMsg = nullptr;
        BEEP(PAT_CLICK);
        if (++calIdx >= 3) {
          TouchCal c;
          if (touchComputeCal(c, S.flip, CAL_X, CAL_Y, calRx, calRy) && calibPlausible(c)) {
            S.cal = c;                 // tymczasowo - zapis dopiero po GOTOWE
            calTest = true;
            calTestAt = millis();
          } else {
            calIdx = 0;
            calMsg = TR("nieudana - jeszcze raz", "failed - once again");
            BEEP(PAT_ALARM);
          }
        }
      }
      touchLock(500);
    }
    calN = 0; calSumX = calSumY = 0;
    fullRedraw = true;
  }
  if (!down) { calN = 0; calSumX = calSumY = 0; }
  was = down;
}

// =====================================================================
//  EKRANY: WiFi, informacje, potwierdzenie, brak dotyku
// =====================================================================
void drawOtaDynamic() {
  auto g = boxBegin(320, 26, C_BG);
  if (!g) return;
  char b[40];
  if (otaProgress < 0) snprintf(b, sizeof(b), "Czekam na plik...");
  else if (otaProgress <= 100) snprintf(b, sizeof(b), "Wgrywanie... %d kB", otaProgress * 15);
  else if (otaProgress == 101) snprintf(b, sizeof(b), "OK - restart");
  else snprintf(b, sizeof(b), TR("BLAD aktualizacji", "Update FAILED"));
  g->setFont(F_SB);
  txtP(g, b, 160, 13, 1, otaProgress == 102 ? C_RED : C_OK);
  boxEnd(0, 196);
}
void drawOta() {
  drawHeader(TR("AKTUALIZACJA WIFI", "WIFI UPDATE"), true);
  if (!otaActive) otaStart();
  char b[48];
  tft.setFont(F_S);
  snprintf(b, sizeof(b), TR("Siec WiFi:  %s", "WiFi network:  %s"), OTA_SSID);   txt(&tft, b, 12, 60, 0, C_WHITE);
  snprintf(b, sizeof(b), TR("Haslo:  %s", "Password:  %s"), OTA_PASS);        txt(&tft, b, 12, 86, 0, C_WHITE);
  tft.setFont(F_M);
  txt(&tft, "http://192.168.4.1", 160, 126, 1, C_TXT);
  tft.setFont(F_S);
  txt(&tft, "Wybierz plik .bin i nacisnij Wgraj", 160, 164, 1, C_DIM);
  drawOtaDynamic();
}

void drawInfo() {
  drawHeader(TR("INFORMACJE", "INFO"), true);
  char b[64];
  int y = 48;
  tft.setFont(F_S);
  auto line = [&](const char *s) { txtFit(s, 8, y, 0, C_WHITE, 306); y += 22; };
  snprintf(b, sizeof(b), "Firmware: v%s  (%s)", FW_VERSION, FW_BUILD); line(b);
  snprintf(b, sizeof(b), TR("Dotyk: %s 0x%02X (SDA %d, SCL %d)", "Touch: %s 0x%02X (SDA %d, SCL %d)"), touchHw.type == TT_GT ? "GT911" : "CST816",
           touchHw.addr, touchHw.sda, touchHw.scl); line(b);
  if (thermo.sensorFound()) {
    const uint8_t *a = thermo.address();
    snprintf(b, sizeof(b), "DS18B20: %02X%02X%02X%02X%02X%02X%02X%02X", a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
  } else snprintf(b, sizeof(b), TR("DS18B20: BRAK (GPIO20)", "DS18B20: NONE (GPIO20)"));
  line(b);
  snprintf(b, sizeof(b), TR("Silnik: TMC2209, %d mA", "Motor: TMC2209, %d mA"), S.currentmA); line(b);
  snprintf(b, sizeof(b), TR("Wolna pamiec: %u B", "Free memory: %u B"), (unsigned)ESP.getFreeHeap()); line(b);
  snprintf(b, sizeof(b), TR("Czas pracy: %lu min", "Uptime: %lu min"), millis() / 60000UL); line(b);
  line(TR("BOOT = START / PAUZA / DALEJ", "BOOT = START / PAUSE / NEXT"));
}

void drawConfirm() {
  tft.fillScreen(C_BG);
  tft.drawRect(16, 40, 288, 120, C_TXT);
  tft.drawRect(17, 41, 286, 118, C_TXT);
  tft.setFont(F_SB);
  txt(&tft, confirmText, 160, 100, 1, C_TXT);
  button(B_NO, 16, 180, 140, 50, TR("NIE", "NO"), C_BTN2, C_WHITE, F_M);
  button(B_YES, 164, 180, 140, 50, TR("TAK", "YES"), C_RED, C_WHITE, F_M);
}

void doConfirm() {
  switch (confirmAct) {
    case CA_RESET_PROG: resetProgram(curProg); goScreen(S_PROG); return;
    case CA_RESET_DEFAULTS: for (int i = 0; i < NUM_DEFAULT; i++) resetProgram(i); break;
    case CA_RESET_CUSTOM: for (int i = NUM_DEFAULT; i < NUM_PROGS; i++) resetProgram(i); break;
    case CA_RESET_SETTINGS:
    case CA_FACTORY:
      settingsDefault(S, true);
      rollStartSec = 30; rollBurstSec = 30; rollBurstN = 4;   // dodatkowe ustawienia ROLL
      saveSettings();
      if (confirmAct == CA_FACTORY) for (int i = 0; i < NUM_PROGS; i++) resetProgram(i);
      backlightSet(S.brightness); lcd.setRotation(1);
      thermo.offset = 0; bootio.soundOn = true; motor.currentmA = S.currentmA;
      break;
    case CA_DEL_STEP: {
      Program &p = progs[curProg];
      for (int i = curStep; i < p.nSteps - 1; i++) { p.s[i] = p.s[i + 1]; rinseChg[curProg][i] = rinseChg[curProg][i + 1]; }
      rinseChg[curProg][p.nSteps - 1] = 0;
      p.nSteps--;
      saveProgram(curProg);
      progDirty = false;
      goScreen(S_PROG);
      return;
    }
  }
  goScreen(confirmReturn);
}

void drawNoTouch() {
  tft.fillScreen(C_BG);
  tft.setFont(F_M);
  txt(&tft, TR("Nie wykryto dotyku", "Touch not detected"), 160, 70, 1, C_RED);
  tft.setFont(F_S);
  txt(&tft, "Dotknij ekranu, aby wybudzic kontroler.", 160, 110, 1, C_WHITE);
  txt(&tft, "Szukam na GPIO 1-5 ...", 160, 136, 1, C_DIM);
  txt(&tft, TR("Szczegoly: Serial Monitor 115200", "Details: Serial Monitor 115200"), 160, 162, 1, C_DIM);
}

// =====================================================================
//  Rysowanie i obsluga przyciskow - rozdzielnia
// =====================================================================
void drawFull() {
  clearBtns();
  nMarq = 0; nPend = 0;
  hdrTemp = false;
  tft.fillScreen(C_BG);
  hdrAccent = C_TXT;
  switch (scr) {
    case S_MAIN: drawMain(); break;
    case S_LIST: drawList(); break;
    case S_PROG: drawProg(); break;
    case S_MENU: drawMenu(); break;
    case S_RUN: drawRun(); break;
    case S_THERMO: drawThermo(); break;
    case S_MOTOR: drawMotor(); break;
    case S_CALIB: drawCalib(); break;
    case S_OTA: drawOta(); break;
    case S_INFO: drawInfo(); break;
    case S_CONFIRM: drawConfirm(); break;
    case S_NOTOUCH: drawNoTouch(); break;
    case S_CAT: drawCat(); break;
    case S_CHEM: drawChem(); break;
    case S_SETUP: drawSetup(); break;
  }
}

void drawDynamic() {
  switch (scr) {
    case S_RUN: drawRunDynamic(); break;
    case S_THERMO: drawThermoDynamic(); break;
    case S_OTA: drawOtaDynamic(); break;
    default: break;
  }
}

void openMenu(MenuId m) { menuId = m; menuPage = 0; goScreen(S_MENU); }

void menuAction(uint8_t act) {
  switch (act) {
    case A_SUB_ROLL: openMenu(M_ROLL); break;
    case A_SUB_STICK: openMenu(M_STICK); break;
    case A_SUB_STAND: openMenu(M_STAND); break;
    case A_SUB_BWRED: openMenu(M_BWRED); break;
    case A_SUB_RESET: openMenu(M_RESET); break;
    case A_CALIB: calibStart(); break;
    case A_MOTOR: motorTestOn = false; goScreen(S_MOTOR); break;
    case A_OTA: goScreen(S_OTA); break;
    case A_INFO: goScreen(S_INFO); break;
    case A_RST_DEF: askConfirm(CA_RESET_DEFAULTS, "Reset programow fabr.?"); break;
    case A_RST_CUST: askConfirm(CA_RESET_CUSTOM, TR("Reset programow wlasnych?", "Reset custom programs?")); break;
    case A_RST_SET: askConfirm(CA_RESET_SETTINGS, TR("Reset ustawien?", "Reset settings?")); break;
    case A_FACTORY: askConfirm(CA_FACTORY, TR("Reset fabryczny?", "Factory reset?")); break;
    case A_STEP_UP: {
      Program &p = progs[curProg];
      if (curStep > 0) {
        Step t = p.s[curStep - 1]; p.s[curStep - 1] = p.s[curStep]; p.s[curStep] = t;
        int16_t r = rinseChg[curProg][curStep - 1]; rinseChg[curProg][curStep - 1] = rinseChg[curProg][curStep]; rinseChg[curProg][curStep] = r;
        curStep--; progDirty = true; fullRedraw = true;
      }
      break;
    }
    case A_STEP_DEL: askConfirm(CA_DEL_STEP, TR("Usunac ten krok?", "Delete this step?")); break;
  }
}

void goBack() {
  switch (scr) {
    case S_LIST: goScreen(listMode == L_COPY ? S_PROG : S_CAT); break;
    case S_CAT: goScreen(S_MAIN); break;
    case S_CHEM: saveChem(curProg); goScreen(S_PROG); break;
    case S_SETUP: runTsrcSave(); if (run.step == 0 && run.phase == R_POUR) goScreen(S_PROG); else goScreen(S_RUN); break;
    case S_PROG: listMode = isCustom(curProg) ? L_CUSTOM : L_DEFAULT; goScreen(S_LIST); break;
    case S_MENU:
      if (menuId == M_STEP) goScreen(S_PROG);
      else if (menuId == M_TSRC) { runTsrcSave(); goScreen(S_RUN); }
      else if (menuId == M_SETTINGS) goScreen(S_MAIN);
      else { menuId = M_SETTINGS; menuPage = 0; goScreen(S_MENU); }
      break;
    case S_MOTOR: case S_OTA: case S_INFO: openMenu(M_SETTINGS); break;
    default: goScreen(S_MAIN);
  }
}

void onButton(uint16_t id) {
  if (id == B_BACK) { goBack(); return; }
  switch (scr) {
    case S_MAIN:
      if (id == B_MAIN0)     { listMode = L_DEFAULT; listPage = 0; goScreen(S_CAT); }
      if (id == B_MAIN0 + 1) { listMode = L_CUSTOM; listPage = 0; goScreen(S_CAT); }
      if (id == B_MAIN0 + 2) goScreen(S_THERMO);
      if (id == B_MAIN0 + 3) openMenu(M_SETTINGS);
      break;
    case S_LIST:
      if (id == B_LIST_UP && listPage > 0) { listPage--; fullRedraw = true; }
      else if (id == B_LIST_DN) { listPage++; fullRedraw = true; }
      else if (id >= B_LIST0 && id < B_LIST0 + LIST_ROWS) {
        int k = listPage * LIST_ROWS + (id - B_LIST0);
        if (k >= listN) break;
        int idx = listIdx[k];
        if (listMode == L_COPY) {                 // kopiuj wybrany program do wlasnego
          if (idx != curProg) {
            char keep[24];
            snprintf(keep, sizeof(keep), "%s*", progs[idx].title);
            progs[curProg] = progs[idx];
            chem[curProg] = chem[idx]; chem[curProg].used = 0; saveChem(curProg);
            memcpy(rinseChg[curProg], rinseChg[idx], sizeof(rinseChg[curProg]));
            strncpy(progs[curProg].title, keep, sizeof(progs[curProg].title) - 1);
            saveProgram(curProg);
          }
          goScreen(S_PROG);
        } else {
          curProg = idx; progScroll = 0;
          // zapamietaj wybor - po wlaczeniu od razu ekran startu tego programu
          prefs.putShort("lastP", curProg); prefs.putShort("lastC", listCat);
          goScreen(S_PROG);
        }
      }
      break;
    case S_PROG: {
      Program &p = progs[curProg];
      if (id == B_PUP && progScroll > 0) { progScroll--; fullRedraw = true; }
      else if (id == B_PDN) { progScroll++; fullRedraw = true; }
      else if (id >= B_PROW0 && id < B_PROW0 + PROW) {
        curStep = progScroll + (id - B_PROW0);
        if (curStep < p.nSteps) openMenu(M_STEP);
      } else if (id == B_PADD && p.nSteps < MAX_STEPS) {
        p.s[p.nSteps] = Step{SN_RINSE, 60, AG_ROLL, CTRL_OFF, 0, 0};
        rinseChg[curProg][p.nSteps] = 0;
        curStep = p.nSteps++;
        saveProgram(curProg);
        openMenu(M_STEP);
      } else if (id == B_PRESET) {
        if (isCustom(curProg)) { listMode = L_COPY; listPage = 0; goScreen(S_LIST); }
        else askConfirm(CA_RESET_PROG, TR("Przywrocic ten program?", "Restore this program?"));
      } else if (id == B_PSTART && p.nSteps > 0) goScreen(S_CHEM);
      break;
    }
    case S_MENU:
      if (id == B_MUP && menuPage > 0) { menuPage--; fullRedraw = true; }
      else if (id == B_MDN && (menuPage + 1) * MROWS < nItems) { menuPage++; fullRedraw = true; }
      else if (id >= B_ITEM0) {
        int r = (id - B_ITEM0) / 4, k = (id - B_ITEM0) % 4;
        int idx = menuPage * MROWS + r;
        if (idx >= nItems) break;
        if (k == 2) {
          if (items[idx].t == IT_ACT) menuAction(items[idx].act);
          else menuChange(r, 1);
        } else menuChange(r, k == 0 ? -1 : 1);
      }
      break;
    case S_RUN: runButton(id); break;
    case S_CHEM: chemButton(id); break;
    case S_SETUP: setupButton(id); break;
    case S_CAT:
      if (id >= B_CAT0 && id < B_CAT0 + 6) { listCat = catFromTile(id - B_CAT0); listPage = 0; goScreen(S_LIST); }
      break;
    case S_THERMO: if (id == B_TRESET) { thermo.resetMinMax(); thermo.histN = 0; } break;
    case S_MOTOR: motorButton(id); break;
    case S_CALIB:
      if (id == B_CDONE) calibAccept();
      if (id == B_CAGAIN) calibStart();
      break;
    case S_CONFIRM:
      if (id == B_YES) doConfirm();
      if (id == B_NO) goScreen(confirmReturn);
      break;
    default: break;
  }
}

// =====================================================================
//  ZRZUTY EKRANU (diagnostyka wygladu) - komenda 'P' w Serial Monitor
//  Kazdy ekran jest rysowany do bufora 320x240 (8 bit RGB332) i wysylany
//  przez USB:  "IMG <nazwa>\n" + 76800 bajtow + "\nEND\n"
// =====================================================================
void sendCapture(lgfx::LGFX_Sprite &cap, const char *name) {
  Serial.printf("IMG %s\n", name);
  const uint8_t *buf = (const uint8_t *)cap.getBuffer();
  for (int off = 0; off < SCR_W * SCR_H; off += 512) {
    Serial.write(buf + off, min(512, SCR_W * SCR_H - off));
    delay(2);
  }
  Serial.print("\nEND\n");
}

void captureOne(const char *name) {
  lgfx::LGFX_Sprite cap(&lcd);
  cap.setColorDepth(8);
  if (!cap.createSprite(SCR_W, SCR_H)) { Serial.println("CAPFAIL"); return; }
  G = &cap;
  drawFull();
  drawDynamic();
  drawHeaderTemp();
  G = &lcd;
  sendCapture(cap, name);
  cap.deleteSprite();
}

void captureAll() {
  Scr saveScr = scr;
  int saveProg = curProg, saveStep = curStep;
  ListMode saveMode = listMode;
  int saveCat = listCat;
  ChemState saveChemS = chem[7];
  RunState saveRun = run;

  scr = S_MAIN;    captureOne("01_menu");
  listMode = L_DEFAULT; scr = S_CAT; captureOne("02_rodzaj");
  listCat = CAT_C41; listPage = 0; scr = S_LIST; captureOne("03_lista_c41");
  curProg = 7; progScroll = 0; scr = S_PROG; captureOne("04_program_adox");
  chem[7].used = 5; scr = S_CHEM; captureOne("05_chemia_uzywana");
  chem[7].used = 0; captureOne("06_chemia_swieza");
  chem[7].used = 5;
  run = RunState(); run.prog = 7; run.step = 0; run.tankRolls = 1; run.phase = R_POUR; scr = S_RUN; captureOne("07a_start_podgrzanie");
  run = RunState(); run.prog = 7; run.step = 1; run.procStart = millis() - 754000UL;
  run.phase = R_POUR; scr = S_RUN; captureOne("07_przebieg_wlej");
  run.timer.start(195, progs[7].s[1].ctrl, true);
  run.timer.tick(70, 36.0f, true, 1.077f);
  run.phase = R_RUN; captureOne("08_przebieg_odliczanie");
  { int16_t sNT = runNoTherm, sV = runVolMl; runNoTherm = 1; runVolMl = 500;
    run.phase = R_POUR; captureOne("08b_czujnik_wyl_wlej");
    run.Tamb = 22.0f; run.phase = R_RUN; captureOne("08c_czujnik_wyl_odliczanie");
    { int ss = run.step; run.step = 0; run.phase = R_RUN; run.timer.start(300, CTRL_OFF, false); run.Twall = 39.4f;
      // realistyczne stany: pokoj 22 C zapisany na starcie, sonda w wodzie / po wylaniu
      float sT = thermo.t; bool sV = thermo.valid, sR = run.roomSet; float sRoom = run.Troom;
      thermo.valid = true; run.roomSet = true; run.Troom = 22.0f;
      waterReset(); wt.everFull = true; wt.maxT = 47.1f; thermo.t = 43.2f; captureOne("08f_woda_plukanie");
      waterReset(); wt.everFull = true; wt.maxT = 47.1f; wt.falling = true; wt.last = 43.2f;
      wt.lo = 31.5f; wt.drainAt = millis() - 12000; thermo.t = 31.5f; run.Twall = 40.8f; captureOne("08g_woda_wylana");
      thermo.t = sT; thermo.valid = sV; run.roomSet = sR; run.Troom = sRoom;
      waterReset(); run.step = ss; }
    { int ss = run.step; run.step = 0; run.phase = R_POUR; scr = S_SETUP; captureOne("08d_przygotowanie"); run.step = ss; } runHoriz = 1; captureOne("08e_przygotowanie_poziomo"); runHoriz = 0; scr = S_RUN;
    runNoTherm = sNT; runVolMl = sV; }
  run.overrunStart = millis() - 4000; run.phase = R_OVERRUN; captureOne("09_przebieg_wylej");
  run.step = 4; run.change = 2; run.phase = R_POUR; captureOne("09b_zmiana_wody");
  run.timer.start(60, CTRL_OFF, false); run.timer.tick(20, NAN, false, 1.0f); run.phase = R_RUN; captureOne("09c_plukanie_odliczanie");
  progScroll = 2; scr = S_PROG; captureOne("04b_program_plukanie"); progScroll = 0; scr = S_RUN;
  run.phase = R_DONE; run.tankRolls = 2; captureOne("10_gotowe");
  menuId = M_SETTINGS; menuPage = 0; scr = S_MENU; captureOne("11_ustawienia_1");
  menuPage = 1; captureOne("12_ustawienia_2");
  menuPage = 2; captureOne("13_ustawienia_3");
  menuPage = 3; captureOne("13b_ustawienia_4");
  curStep = 1; menuId = M_STEP; menuPage = 0; captureOne("14_edycja_kroku");
  menuPage = 1; captureOne("14b_edycja_kroku_2");
  menuId = M_ROLL; menuPage = 0; captureOne("15_agitacja_roll");
  scr = S_THERMO; captureOne("16_termometr");
  scr = S_MOTOR; captureOne("17_test_silnika");
  scr = S_INFO; captureOne("18_informacje");
  strcpy(confirmText, TR("Przywrocic ten program?", "Restore this program?")); scr = S_CONFIRM; captureOne("19_potwierdzenie");
  calIdx = 0; calTest = false; scr = S_CALIB; captureOne("20_kalibracja");
  listMode = L_CUSTOM; scr = S_CAT; captureOne("21_rodzaj_wlasne");

  chem[7] = saveChemS; run = saveRun;
  curProg = saveProg; curStep = saveStep; listMode = saveMode; listCat = saveCat;
  menuId = M_SETTINGS; menuPage = 0;
  scr = saveScr;
  fullRedraw = true;
  Serial.println("CAPDONE");
}
