# Spool Film Processor – instrukcja krok po kroku

Spool Film Processor – procesor do wywoływania filmów na **ESP32-C3 z ekranem dotykowym 2,4" (msalamon EVB-00055)**.

```
spool-film-processor\
├── SpoolFilmProcessor\          <- program (szkic Arduino)
│   ├── SpoolFilmProcessor.ino   setup() / loop()
│   ├── config.h               WSZYSTKIE piny i stale (tu zmieniasz sprzet)
│   ├── compensation.h         ALGORYTM kompensacji czasu od temperatury + krzywe
│   ├── programs.h             programy fabryczne, ustawienia, zapis w pamieci
│   ├── motor.h                TMC2209 przez UART (1 pin) + tryby agitacji
│   ├── thermo.h               DS18B20
│   ├── touch.h                dotyk z autowykrywaniem pinow + kalibracja
│   ├── bootio.h               przycisk BOOT (START) + brzeczyk
│   ├── ota.h                  aktualizacja przez WiFi
│   ├── display.h              konfiguracja ekranu ST7789
│   └── ui.h                   wszystkie ekrany
├── schemat.html               schemat polaczen (otworz w przegladarce)
└── INSTRUKCJA.md              ten plik
```

---

## 1. Co robi urządzenie

| Funkcja | W tym projekcie |
|---|---|
| Programy fabryczne (B&W, C-41, E-6, ECN-2, RA-4) | 7 programów: 1.1 B&W ogólny, 1.2 DF96, 2.1 C-41 Tetenal, 2.2 C-41 Cinestill, 3.1 E-6, 4.1 ECN-2, 5.1 RA-4 |
| Programy własne | 4 sloty W1–W4, przycisk **KOPIUJ** tworzy kopię dowolnego programu |
| Edycja kroku: nazwa, czas, agitacja, kompensacja | Tak, wszystko na ekranie dotykowym |
| Kompensacja czasu od temperatury w trakcie procesu | Tak, z całkowaniem w czasie rzeczywistym (patrz rozdział 7) |
| MON: pilnowanie zakresu temperatury | Tak, termometr na czerwonym tle poza zakresem |
| Agitacja ROLL / STICK / STAND / brak | Tak (domyślnie 60 rpm, 15/14 s itd.) |
| Silnik pracuje 15 s po wyzerowaniu czasu | Tak, potem przejście do następnego kroku |
| Automatyczna redukcja czasu DEV dla B&W (15%) | Tak (Ustawienia → Redukcja czasu B&W) |
| Dźwięk, jasność, °C/°F, reset programów/ustawień | Tak |
| Aktualizacja przez WiFi | Tak (Ustawienia → Aktualizacja WiFi) |
| — | **Obrót ekranu o 180°** (Ustawienia), dotyk obraca się razem z ekranem |
| — | Przycisk **BOOT** na płytce = START / PAUZA / DALEJ (działa mokrymi palcami) |

---

## 2. Instalacja oprogramowania na PC (Windows)

1. **Arduino IDE 2.x**: pobierz z <https://www.arduino.cc/en/software> i zainstaluj (zgódź się na instalację sterowników).
2. Uruchom Arduino IDE → **File → Preferences** → pole *Additional boards manager URLs* → wklej:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   → OK.
3. **Tools → Board → Boards Manager** → wyszukaj `esp32` → **esp32 by Espressif Systems** → zainstaluj wersję **3.x** (najnowszą 3.x).
4. **Tools → Manage Libraries** (Library Manager) → zainstaluj:
   - **LovyanGFX** (autor: lovyan03)
   - **OneWire** (Paul Stoffregen)
   - **DallasTemperature** (Miles Burton). Przy pytaniu o zależności wybierz *Install all*.
5. **File → Open** → `spool-film-processor\SpoolFilmProcessor\SpoolFilmProcessor.ino`
   (w zakładkach otworzą się wszystkie pliki `.h`).

## 3. Ustawienia płytki w Arduino IDE

Menu **Tools**:

| Opcja | Wartość |
|---|---|
| Board | **ESP32C3 Dev Module** |
| USB CDC On Boot | **Enabled** ← obowiązkowe, inaczej kompilacja zatrzyma się z komunikatem |
| CPU Frequency | 160 MHz |
| Flash Size | 4MB |
| Partition Scheme | **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** |
| Upload Speed | 921600 |
| Port | COMx, który pojawi się po podłączeniu płytki |

> Dlaczego *USB CDC On Boot = Enabled*? Komunikaty debug idą wtedy przez USB-C. Piny GPIO20/21 (RXO/TXO) zostają wolne dla czujnika i silnika.

## 4. Wgrywanie programu

1. Podłącz **samą płytkę ESP32** kablem USB-C do PC. Kabel musi przesyłać dane, nie tylko ładować.
2. Wybierz port w **Tools → Port**. Płytka używa wbudowanego USB ESP32-C3, więc nie potrzebuje sterownika CH340.
3. Kliknij **Upload (→)**.
4. Jeśli wgrywanie się nie zaczyna (`Failed to connect`/brak portu):
   przytrzymaj **BOOT**, krótko naciśnij **RESET** (复位), puść **BOOT** i ponów Upload. Po wgraniu naciśnij **RESET**.
5. Otwórz **Tools → Serial Monitor**, ustaw **115200**. Zobaczysz np.:
   ```
   [FILM PROCESSOR] start, FW 1.0
   [TOUCH] znaleziono: adres 0x15, SDA=GPIO4, SCL=GPIO5, typ=CST816/FT6x36
   [TEMP] DS18B20 znaleziony: 28FF....
   ```

> Kod nie był jeszcze kompilowany na Twoim komputerze. Jeśli kompilator zgłosi błąd, skopiuj **całą** czerwoną treść błędu i mi wyślij, poprawię.

Późniejsze aktualizacje możesz wgrywać bez kabla: **Sketch → Export Compiled Binary**, potem na urządzeniu **Ustawienia → Aktualizacja WiFi**. Połącz telefon/PC z siecią `FilmProcessor` (hasło `film1234`), wejdź na `http://192.168.4.1` i wybierz plik `SpoolFilmProcessor.ino.bin` z folderu `build`.

---

## 5. Testowanie – etapami (zawsze od najprostszego)

### Etap A – sama płytka (zasilanie z USB komputera)
1. Po wgraniu pojawi się ekran **FILM PROCESSOR**.
2. **Dotyk:** program sam szuka kontrolera dotyku na GPIO1–5.
   - Jeśli pokaże *„Nie wykryto dotyku”*, dotknij ekranu. Kontroler CST816 usypia się i budzi dotknięciem.
   - Znalezione piny zapisze w pamięci, więc następny start jest szybszy.
3. **Kalibracja** (przy pierwszym starcie automatycznie): dotknij dokładnie środka 3 krzyżyków.
   Potem ekran testowy: dotykaj, a zielone kropki powinny pojawiać się pod palcem. Naciśnij **GOTOWE**.
   - Kalibrację powtórzysz: *Ustawienia → Kalibracja dotyku* albo **przytrzymując palec na ekranie (lub BOOT) zaraz po włączeniu**.
4. **Obrót ekranu:** *Ustawienia → Obrot ekranu 180°*. Dotyk obraca się automatycznie i nie trzeba ponownie kalibrować.
5. Przejdź po menu i otwórz program *2.1 C-41 Tetenal*, wejdź w krok i zmień czas. Wyjdź, wyłącz zasilanie, włącz ponownie i sprawdź, czy zmiana została zapamiętana.

### Etap B – czujnik DS18B20
1. Podłącz wg schematu: czerwony → 3.3V, czarny → GND, żółty → RXO (GPIO20), **4,7 kΩ między żółtym a 3.3V**.
2. *Menu → Termometr*: powinna pojawić się temperatura. Wartość `--.-` oznacza brak czujnika albo brak rezystora 4,7 kΩ.
3. Włóż czujnik do wody razem z dobrym termometrem kuchennym. Różnicę wpisz w *Ustawienia → Korekta czujnika*.

### Etap C – zasilanie (bez ESP i bez silnika!)
1. Moduł ładowania + akumulator: zmierz polaryzację padów i napięcie 5 V na wyjściu.
2. **XL6009**: zasil 5 V i bez obciążenia ustaw potencjometrem **12,0 V** na wyjściu. Dopiero wtedy podłączaj go do TMC2209.

### Etap D – TMC2209 i silnik
1. Podłącz TMC2209 wg schematu (VIO→3.3V, +VMOT→12V + kondensator 1000 µF, ENABLE/MS1/MS2/SPREAD/STEP/DIR→GND, PDN→1 kΩ→TXO).
2. Silnik podłączaj i odłączaj **tylko przy wyłączonym zasilaniu**.
3. *Ustawienia → Test silnika → START*. Silnik powinien obracać się płynnie i cicho (tryb stealthChop).
   - Przyciskami −/+ zmieniasz obroty i prąd. Zacznij od **600–700 mA**.
   - Silnik staje pod obciążeniem (pełny koreks)? Zwiększ prąd o 100 mA.
   - Silnik parzy w rękę (> 60 °C)? Zmniejsz prąd.
   - Silnik tylko „brzęczy” i drga? Jedna cewka jest źle podłączona: sprawdź pary miernikiem (~3 Ω).
   - Kręci się w złą stronę? `config.h`: `MOTOR_INVERT true`.
4. Komunikacja z TMC2209 jest **jednokierunkowa** (ESP tylko wysyła). Jeśli silnik w ogóle nie reaguje, sprawdź kolejno: 12 V na +VMOT, 3,3 V na VIO, rezystor 1 kΩ, czy przewód idzie do pinu PDN (nie USART/RX) i czy ENABLE/MS1/MS2 są na GND.

### Etap E – próba „na sucho” z wodą
1. Napełnij koreks wodą o temperaturze ok. 30 °C.
2. Uruchom *2.1 C-41 Tetenal*: **START** na kroku HEAT, potem **POMIŃ** (dotknij dwa razy).
3. Krok DEV: czas nominalny 3:15 przy 38 °C. Przy 30 °C program powinien pokazać ok. **+4:45** (czas ~8:00) i mnożnik **x2.46** na szarym/czerwonym panelu temperatury.
4. Dolej ciepłej wody, a pozostały czas powinien się skrócić. To jest kompensacja w czasie rzeczywistym.

---

## 6. Obsługa

1. **Menu główne → Programy fabryczne → wybierz program.**
2. Ekran programu: lista kroków `NAZWA  CZAS  AGITACJA  KOMPENSACJA  TEMP`.
   Dotknij kroku, żeby go edytować. **RESET** przywraca program fabryczny. W programach własnych w tym miejscu jest **KOPIUJ**.
3. **START** → ekran *„WLEJ: Wywoływacz”*. Wlej chemię i naciśnij **START** (lub BOOT).
4. W trakcie widzisz: pozostały czas, czas dodany przez kompensację (`+0:42`), temperaturę (szare tło = w normie, czerwone = poza zakresem) i mnożnik czasu.
   **PAUZA**, **POMIŃ** i **STOP** (te dwa ostatnie wymagają podwójnego dotknięcia).
5. Po wyzerowaniu: alarm, *„WYLEJ”*, silnik pracuje jeszcze 15 s, potem urządzenie przechodzi do następnego kroku. **DALEJ** pomija te 15 s.
6. Po ostatnim kroku pojawia się *„GOTOWE!”* z całkowitym czasem procesu.

**Agitacja (Ustawienia):**
- ROLL – obroty ciągłe, T1 s w prawo, T2 s w lewo (domyślnie 60 rpm, 15/14 s)
- STICK – T1 s ciągle, potem T2 s co minutę (60 rpm, 60/10 s)
- STAND – T2 s na starcie, potem co T1 minut T3 s obrotów (60 rpm, 30 min/60 s/30 s)

Kompensacja jest wyłączona dla STICK i STAND, bo w pionie czujnik nie dotyka chemii.

---

## 7. Algorytm kompensacji czasu od temperatury

Plik: `compensation.h`.

**Model.** Szybkość wywoływania rośnie wykładniczo z temperaturą (prawo Arrheniusa).
Dla każdej chemii jest krzywa współczynnika:

```
F(T) = czas_przy_T / czas_przy_T_nominalnej        F(T_nom) = 1,00
```

zapisana jako kilka punktów z tabel producentów. Między punktami interpolacja jest **liniowa w logarytmie**,
co daje dokładnie przebieg wykładniczy:

```
ln F(T) = ln F1 + (ln F2 − ln F1) · (T − T1) / (T2 − T1)
```

Poza tabelą krzywa jest przedłużana o maks. 5 °C, a F ograniczone do 0,25–4,0.

**Całkowanie w czasie rzeczywistym.** Chemia stygnie w trakcie kroku, więc czasu nie liczymy raz na starcie.
Co ok. 15 ms program dodaje „postęp wywołania”:

```
postęp += Δt / (t_nom · F(T_teraz))         krok kończy się przy postęp ≥ 1
pozostało  = (1 − postęp) · t_nom · F(T_teraz)
dodany czas = (upłynęło + pozostało) − t_nom      → wyświetlane jako „+0:42”
```

**Przykład (C-41 Tetenal, 3:15 przy 38,0 °C):**

| Temperatura | F | Czas kroku DEV |
|---|---|---|
| 39 °C | 0,92 | 2:59 |
| 38 °C | 1,00 | 3:15 |
| 36 °C | 1,29 | 4:12 |
| 34 °C | 1,62 | 5:16 |
| 30 °C | 2,46 | 8:00 |

Jeśli połowa kroku przebiegnie przy 36 °C, a druga połowa przy 35 °C, krok potrwa 2:06 + 2:21 = **4:27**.

**Krzywe w programie** (przybliżone na podstawie ogólnodostępnych tabel, **zweryfikuj z kartą swojej chemii**):

| Krzywa | T nom | Zakres „w normie” | Źródło |
|---|---|---|---|
| BW – B&W ogólny | 20 °C | 18–24 °C | wykres Ilford (≈ −9,5%/°C) |
| DF96 | 26,7 °C | 21–30 °C | Cinestill DF96 |
| C41T – C-41 Tetenal/Bellini | 38 °C | 30–40 °C | Tetenal Colortec |
| C41C – C-41 Cinestill/Arista | 39 °C | 24–41 °C | Cinestill Cs41 (102°F 3:30, 80°F 8:00) |
| BLIX – C-41/E-6 | 38 °C | 30–40 °C | mniej wrażliwy |
| E6FD – E-6 1. wyw. | 38 °C | 36–40 °C | bardzo wrażliwy |
| E6CD – E-6 barwny | 38 °C | 35–40 °C | |
| ECN2 | 41,1 °C | 30–43 °C | Kodak SM-2 |
| RA4 | 35 °C | 25–38 °C | |

**Jak dodać/poprawić krzywą swojej chemii:** z tabeli producenta weź kilka par (temperatura, czas) i podziel każdy czas przez czas przy temperaturze nominalnej. Wpisz punkty jako `{temp×10, F×1000}`, na przykład:

```cpp
// mój wywoływacz: 20°C 8:00, 24°C 5:30 → F(24) = 330/480 = 0,688
static const CurvePt C_MOJ[] = { {180, 1220}, {200, 1000}, {240, 688} };
// i w tablicy CURVES[]:
{"Moj wywolywacz", "MOJ", 200, 180, 240, C_MOJ, N_(C_MOJ)},
```

Nowa krzywa pojawi się w edycji kroku jako opcja „Kontrola czasu”.

---

## 8. Zasilanie i włącznik

- **ESP32 nie może się całkowicie wyłączyć.** Potrafi tylko uśpić procesor, a ekran, TMC2209 i XL6009 nadal pobierałyby prąd. Dlatego jest **włącznik SW1**.
- SW1 wstaw **między wyjście 5V modułu ładowania a resztę układu**, nie między akumulator i ładowarkę. Wtedy:
  - wyłączone urządzenie nic nie pobiera z akumulatora,
  - ładowanie przez USB-C działa także przy wyłączonym urządzeniu.
- Te moduły (np. z układem IP5306) potrafią same wyłączyć wyjście przy bardzo małym poborze prądu (< ~50 mA przez ~30 s). Jeśli urządzenie gaśnie samo w menu, podłącz przycisk do padu **K** i GND (krótkie wciśnięcie włącza wyjście) albo ustaw większą jasność ekranu.
- Pomiar poziomu baterii nie jest możliwy, bo nie ma wolnego pinu. Poziom pokazują diody na module ładowania.

## 9. Problemy i rozwiązania

| Objaw | Przyczyna / rozwiązanie |
|---|---|
| `#error ... USB CDC On Boot` przy kompilacji | Tools → USB CDC On Boot → **Enabled** |
| `#error ... wersji 3.x` | zaktualizuj *esp32 by Espressif* do 3.x w Boards Manager |
| `Sketch too big` | Tools → Partition Scheme → **Minimal SPIFFS (1.9MB APP with OTA)** |
| Brak portu COM | inny kabel (z transmisją danych), tryb BOOT+RESET (rozdział 4) |
| Czarny ekran | sprawdź podświetlenie (GPIO0). Odwrócone kolory: `TFT_INVERT false` w `config.h` |
| „Nie wykryto dotyku” | dotknij ekranu. Jeśli nadal nic, wyślij mi log z Serial Monitor |
| Dotyk trafia obok | Ustawienia → Kalibracja dotyku albo przytrzymaj palec przy starcie |
| Temperatura `--.-` | brak rezystora 4,7 kΩ, zamienione przewody, zły pin (musi być RXO) |
| Silnik nie rusza | 12 V na +VMOT? 3,3 V na VIO? PDN (nie USART!) przez 1 kΩ do TXO? ENABLE, MS1, MS2 na GND? Wspólna masa? |
| Silnik drga/brzęczy | źle połączone cewki: pary sprawdź miernikiem |
| Wyłącza się w menu | patrz rozdział 8 (auto-off modułu ładowania) |

---

## 10. Pliki do zmiany pod Twój sprzęt (`config.h`)

| Stała | Znaczenie |
|---|---|
| `PIN_TOUCH_SDA / SCL` | gdy znasz piny dotyku (zamiast autowykrywania) |
| `BUZZER_ENABLED`, `BUZZER_PASSIVE` | głośnik na GPIO9 przez wzmacniacz LM386 (schemat.html, rozdz. 3) |
| `TMC_RSENSE` | rezystory pomiarowe na module TMC2209 (zwykle 0.11) |
| `GEAR_RATIO` | jeśli między silnikiem a koreksem jest przekładnia |
| `MOTOR_INVERT` | odwrócenie kierunku |
| `OVERRUN_SEC` | ile sekund silnik pracuje po wyzerowaniu (domyślnie 15) |
| `OTA_SSID / OTA_PASS` | nazwa i hasło sieci do aktualizacji |
