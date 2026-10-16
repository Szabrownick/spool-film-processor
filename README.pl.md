# Spool Film Processor

[English](README.md) · **Polski**

Samodzielnie zbudowany procesor do wywoływania filmów (C-41, B&W, E-6, ECN-2, RA-4). Mieszaniem w koreksie steruje silnik krokowy. Całą obsługę zapewnia mały ekran dotykowy (2,4", ok. 49×38 mm). Program sam pilnuje czasu kroków i koryguje go według temperatury chemii i jej zużycia.

**Wersja programu: v2.1**

<p align="center">
  <img src="docs/screens/00_intro.png" width="320" alt="Ekran startowy">
  <img src="docs/screens/07_pour_developer.png" width="320" alt="Wlewanie wywoływacza">
</p>

> 🖨️ **Pliki do druku 3D** (obudowa: front z PETG, tył z PLA-CF, gniazdo koreksu, mieszadło) zostaną udostępnione na **[MakerWorld](https://makerworld.com/)** w późniejszym czasie. Link pojawi się tutaj.

---

## Spis treści
- [Jak to działa](#jak-to-działa)
- [Zdjęcia](#zdjęcia)
- [Przebieg procesu – ekrany](#przebieg-procesu--ekrany)
- [Kompensacja temperatury i zużycia chemii](#kompensacja-temperatury-i-zużycia-chemii)
- [Tryb „czujnik nie w chemii”](#tryb-czujnik-nie-w-chemii)
- [Programy i ustawienia](#programy-i-ustawienia)
- [Sprzęt](#sprzęt)
- [Wgrywanie programu](#wgrywanie-programu)
- [Struktura repozytorium](#struktura-repozytorium)

---

## Jak to działa

1. **Koreks** (np. Paterson Multi Reel 3, czyli 3×35 mm albo 2×120) stoi w gnieździe urządzenia. Silnik przez mieszadło obraca spirale z filmem.
2. **Wybierasz program**, np. „ADOX C-TEC 41”. Program to lista kroków: podgrzanie, wywoływacz, płukanie, blix, płukanie, stabilizator. Każdy krok ma czas, sposób mieszania i sposób kontroli temperatury.
3. Przed startem urządzenie pyta o **ustawienie**: czy procesor stoi pionowo czy leży, czy czujnik dotyka chemii, ile jest chemii i jaką temperaturę ma woda do kąpieli wstępnej.
4. Na każdym kroku ekran mówi, **co wlać**. Po naciśnięciu **START** (na ekranie albo przycisk BOOT) silnik zaczyna mieszać, a zegar odlicza czas.
5. **Czas jest korygowany na bieżąco.** Gdy chemia jest chłodniejsza od nominalnej albo już używana, krok się wydłuża. O ile, widać na ekranie: „+1:08” oraz mnożnik, np. „x1.35”.
6. Po końcu kroku melodia sygnalizuje **„WYLEJ”**. Silnik staje, a przez 15 s trwa odliczanie na ocieknięcie. Potem urządzenie przechodzi do kolejnego kroku.
7. Na końcu pojawia się **„GOTOWE!”**. Podajesz liczbę rolek w koreksie, a urządzenie zapamiętuje zużycie chemii. Przy kolejnym procesie automatycznie wydłuży czas wywoływacza i blixu.

Po włączeniu urządzenie od razu otwiera **ostatnio użyty program**. Przycisk wstecz prowadzi do listy i wyboru rodzaju procesu.

---

## Zdjęcia

| | |
|---|---|
| <img src="docs/photos/01_finished_front.jpg" width="400"> | <img src="docs/photos/02_tank_seat_agitator.jpg" width="400"> |
| Gotowe urządzenie: front z PETG z ekranem dotykowym i wskaźnikiem baterii, uchwyt na górze. | Gniazdo koreksu z rowkiem na O-ring, wałek mieszadła i bolec ustalający. |
| <img src="docs/photos/03_in_use_prewash.jpg" width="400"> | <img src="docs/photos/04_assembly_display.jpg" width="400"> |
| W użyciu przy zlewie: płukanie, czujnik mierzy wodę z kranu (41,5 °C). | Montaż: ekran i płytka ESP32-C3 na miejscu, ekran START. |
| <img src="docs/photos/05_enclosure_motor_mount.jpg" width="400"> | <img src="docs/photos/06_electronics.jpg" width="400"> |
| Obudowa z mocowaniem silnika NEMA17. | Elektronika: TMC2209 z radiatorem, XL6009, moduł ładowania, LM386 na płytce uniwersalnej, akumulator. |

<p align="center"><img src="docs/photos/00_fusion360_model.png" width="820" alt="Model Fusion 360"><br><i>Model w Fusion 360 – pliki do druku wkrótce na MakerWorld.</i></p>

---

## Przebieg procesu – ekrany

Zrzuty ekranu pochodzą prosto z urządzenia (ekran 320×240, na zrzutach powiększony 2×). Pokazują wersję angielską interfejsu; polska wygląda tak samo (Ustawienia → Język).

### Wybór programu

| | |
|---|---|
| <img src="docs/screens/01_menu.png" width="320"> | <img src="docs/screens/02_process_type.png" width="320"> |
| **Menu główne:** programy fabryczne, własne, termometr i ustawienia. | **Rodzaj procesu:** kafle C-41, B&W, E-6, ECN-2 i RA-4 z liczbą programów. |
| <img src="docs/screens/03_list_c41.png" width="320"> | <img src="docs/screens/04_program_adox.png" width="320"> |
| **Lista programów** z łącznym czasem, zestawem chemii i liczbą zużytych rolek. | **Program:** kroki z czasem, mieszaniem (ROLL) i kontrolą temperatury (krzywa ADOX, MON = tylko podgląd). |
| <img src="docs/screens/05_chem_used.png" width="320"> | <img src="docs/screens/04b_program_rinse.png" width="320"> |
| **Chemia:** świeża lub używana, liczba zużytych rolek, zestaw (ADOX, Tetenal, Bellini, Cinestill…). Na dole widać, o ile wydłuży się wywoływacz i blix. | **Płukanie zmianami wody**, np. „6x ROLL”: 6 wymian wody w czasie kroku. |

### Przygotowanie i start

| | |
|---|---|
| <img src="docs/screens/08d_setup.png" width="320"> | <img src="docs/screens/08e_setup_lying.png" width="320"> |
| **START:** pionowo albo poziomo, czy czujnik dotyka chemii, ilość chemii, temperatura kąpieli wstępnej. Pod spodem przewidywana temperatura koreksu przy wlewaniu chemii. | Ustawienia są **zapamiętywane dla każdego programu osobno**, więc zwykle wystarczy nacisnąć DALEJ. |
| <img src="docs/screens/07a_start_preheat.png" width="320"> | <img src="docs/screens/08f_water_prewash.png" width="320"> |
| **Pierwszy krok:** u góry „rolka 6/16” (kontrola zużycia chemii), na dole szacowany czas całego procesu. | **Kąpiel wstępna:** czujnik mierzy wodę z kranu (ostatni szczyt, maksimum). Kolor zielony oznacza zakres 38–45 °C. |

### W trakcie procesu

| | |
|---|---|
| <img src="docs/screens/07_pour_developer.png" width="320"> | <img src="docs/screens/08_countdown.png" width="320"> |
| **WLEJ: Wywoływacz** – szacowany czas kroku, pozostały czas całego procesu i temperatura. | **Odliczanie:** duży zegar, pasek postępu, wydłużenie („+1:08”) i łączny mnożnik czasu. Na dole co 3 s zmienia się opis: temperatura / chemia. |
| <img src="docs/screens/08b_probe_off_pour.png" width="320"> | <img src="docs/screens/08c_probe_off_countdown.png" width="320"> |
| **Czujnik nie w chemii:** temperatura pokoju, temperatura koreksu z modelu i prognoza kroku („37.8>36.9°”, „+0:24”). | W trakcie kroku duża liczba to **temperatura chemii z modelu cieplnego**. |
| <img src="docs/screens/09_drain.png" width="320"> | <img src="docs/screens/09b_water_change.png" width="320"> |
| **WYLEJ:** silnik stoi, odliczanie 15 s na ocieknięcie, podpowiedź kolejnego kroku. | **Zmiana wody** przy płukaniu zmianami (3 z 6). |
| <img src="docs/screens/09c_rinse_countdown.png" width="320"> | <img src="docs/screens/10_done.png" width="320"> |
| Odliczanie jednej zmiany wody. | **GOTOWE:** łączny czas i liczba rolek w koreksie; zużycie chemii zapisuje się po ZAPISZ. |

### Ustawienia i narzędzia

| | |
|---|---|
| <img src="docs/screens/11_settings_1.png" width="320"> | <img src="docs/screens/12_settings_2.png" width="320"> |
| Język PL / EN, dźwięk, pikanie co minutę, jasność. | Obrót ekranu 180°, jednostki °C / °F, parametry mieszania. |
| <img src="docs/screens/13_settings_3.png" width="320"> | <img src="docs/screens/13b_settings_4.png" width="320"> |
| Prąd silnika, korekta czujnika, redukcja czasu B&W. | Kalibracja dotyku, test silnika, aktualizacja przez WiFi, reset. |
| <img src="docs/screens/14_step_edit.png" width="320"> | <img src="docs/screens/15_roll_agitation.png" width="320"> |
| **Edycja kroku:** nazwa, czas, zmiany wody, mieszanie, kontrola temperatury. | **Mieszanie ROLL:** prędkość i czasy obrotów w prawo i w lewo. |
| <img src="docs/screens/16_thermometer.png" width="320"> | <img src="docs/screens/17_motor_test.png" width="320"> |
| **Termometr** z minimum i maksimum. | **Test silnika** – prędkość, prąd, kierunek. |
| <img src="docs/screens/20_calibration.png" width="320"> | <img src="docs/screens/18_info.png" width="320"> |
| **Kalibracja dotyku:** 3 krzyżyki. Uruchamia się dotknięciem ekranu w trakcie logo. | **Informacje:** wersja programu z datą kompilacji, czujniki, pamięć. |

---

## Kompensacja temperatury i zużycia chemii

**Temperatura.** Każdy typ chemii ma krzywą „czas względem temperatury” (np. ADOX C-TEC 41: 38 °C = 1,0×, 30 °C ≈ 2,46×). Program co chwilę liczy współczynnik dla aktualnej temperatury i całkuje postęp kroku:

```
postęp += dt / (czas_bazowy × F(T) × zużycie)
```

Jeśli chemia w trakcie kroku stygnie, krok wydłuża się dokładnie o tyle, ile trzeba. Nie jest to jednorazowe przeliczenie na starcie.

**Zużycie chemii.** Dla zestawów z tabelą producenta (ADOX, Tetenal: grupy po 4 rolki przy 1 L) czas wywoływacza i blixu rośnie z każdą rolką. Wartości między tabelami 38 °C i 30 °C są interpolowane według temperatury. Bellini: +15 s co 3 rolki. Cinestill i zestaw ogólny: +2% na rolkę. Licznika nie trzeba resetować ręcznie: wybór „świeża” na ekranie chemii go zeruje, a po przekroczeniu wydajności podanej przez producenta pojawia się „!”.

## Tryb „czujnik nie w chemii”

Przy małej ilości chemii (np. Paterson na stojąco z 3 rolkami) sonda nie sięga cieczy i mierzy powietrze. Wtedy:

- **temperatura pokoju** jest zapisywana raz, na starcie;
- w **kąpieli wstępnej i płukaniu** sonda działa jak **termometr wody z kranu**: wykrywa kontakt z wodą, zapamiętuje ostatni szczyt i maksimum, pokazuje „ustalona”, gdy koreks jest pełny, i **pika**, gdy woda jest za gorąca (> 45 °C) albo za zimna (< 38 °C);
- **temperaturę chemii liczy model cieplny koreksu**: ciecz ↔ ścianki i spirale ↔ pokój. Uwzględnia konwekcję naturalną (korelacje Churchilla–Chu dla ściany pionowej i walca poziomego), promieniowanie, przewodzenie przez ściankę z polistyrenu oraz wodę pozostałą w koreksie. Model działa w czasie rzeczywistym, także gdy pusty koreks czeka na chemię;
- przykład (500 ml, pionowo, pokój 21 °C, ADOX 3:15): koreks po kąpieli wodą 45 °C → chemia 38,0 → 37,7 °C, korekta 0…+3 s. Zimny koreks → +33 s.

## Programy i ustawienia

- 8 programów fabrycznych (C-41 Tetenal / Cinestill / ADOX, B&W, E-6, ECN-2, RA-4) i 4 własne (kopiowanie z fabrycznych).
- Mieszanie **ROLL** (obroty w prawo i w lewo), **STICK**, **STAND** (stand development) albo brak.
- Kontrola czasu kroku: krzywa temperaturowa, **MON** (tylko podgląd zakresu) albo wyłączona.
- Wszystko jest zapisywane w pamięci płytki: ustawienia, kalibracja, programy, zużycie chemii i ostatni wybór.
- Komendy przez port szeregowy (115200): `i` informacje, `t` temperatura, `r`/`s` silnik, `a`/`f`/`h`/`c` dźwięki, `K` tabela modelu cieplnego, `P` zrzuty ekranu.

---

## Sprzęt

| Element | Model |
|---|---|
| Sterownik + ekran | ESP32-C3 z TFT 2,4" ST7789 i dotykiem CST816 (msalamon EVB-00055) |
| Silnik | JK42HS48-1204 (NEMA17) + TMC2209 (Tenstar V985) przez UART na jednym przewodzie |
| Temperatura | DS18B20, wodoodporny |
| Zasilanie | Li-ion 3,7 V 5000 mAh, moduł ładowania USB-C 5 V 4 A, XL6009 → 12 V |
| Dźwięk | LM386 + głośnik 14 Ω |
| Uszczelnienia | O-ring 131×3 NBR70 (gniazdo koreksu), simmering 8×16×5 TC na tulejce 8×1,5 |

Płytka ma wyprowadzone tylko dwa wolne piny: GPIO21 steruje silnikiem (UART), a GPIO20 czujnikiem (1-Wire). Przycisk BOOT (GPIO9) to START / PAUZA / DALEJ i jednocześnie sygnał dla głośnika.

Pełna lista części (BOM), schematy, tabele połączeń, montaż i uszczelnienia: **[SpoolFilmProcessor_dokumentacja_v2.1.pdf](docs/pl/SpoolFilmProcessor_dokumentacja_v2.1.pdf)** (źródło: [docs/pl/schemat.html](docs/pl/schemat.html)).

---

## Wgrywanie programu

| Sposób | Plik | Ustawienia na płytce |
|---|---|---|
| Arduino IDE | folder [SpoolFilmProcessor](SpoolFilmProcessor) albo [SpoolFilmProcessor_v2.1_ArduinoIDE.zip](firmware/SpoolFilmProcessor_v2.1_ArduinoIDE.zip) | zostają |
| WiFi (Ustawienia → Aktualizacja WiFi, sieć `FilmProcessor`, http://192.168.4.1) | [firmware/SpoolFilmProcessor_v2.1_update.bin](firmware/SpoolFilmProcessor_v2.1_update.bin) | zostają |
| USB, aktualizacja | `firmware/flash_update.bat` | zostają |
| USB, nowa płytka (adres 0x0) | `firmware/flash_full.bat` / [SpoolFilmProcessor_v2.1_full.bin](firmware/SpoolFilmProcessor_v2.1_full.bin) | kasowane |

Pliki `.bat` potrzebują `esptool.exe` w folderze `firmware`. Pobierz `esptool-windows-amd64.zip` z [espressif/esptool releases](https://github.com/espressif/esptool/releases). Nową płytkę można też wgrać bez `.bat`: [esptool-js](https://espressif.github.io/esptool-js/) w Chrome lub Edge, adres `0x0`.

**Arduino IDE:** pakiet `esp32` (Espressif) 3.x, biblioteki LovyanGFX, OneWire i DallasTemperature. W menu Tools: *ESP32C3 Dev Module*, *USB CDC On Boot: Enabled*, *Partition Scheme: Minimal SPIFFS (1.9MB APP with OTA)*. Szczegóły: [docs/pl/INSTRUKCJA.md](docs/pl/INSTRUKCJA.md).

Domyślne hasło sieci do aktualizacji przez WiFi to `film1234`. Zmienisz je w [config.h](SpoolFilmProcessor/config.h) (`OTA_PASS`).

## Struktura repozytorium

```
SpoolFilmProcessor/   kod źródłowy (Arduino)
firmware/           gotowe pliki .bin, skrypty wgrywania, ZIP dla Arduino IDE
docs/screens/       zrzuty ekranu z urządzenia
README.md           README po angielsku (domyślny)
docs/photos/        zdjęcia urządzenia i model z Fusion 360
docs/pl/            dokumentacja (BOM, schematy, montaż, uszczelnienia, obsługa), PDF i instrukcja po polsku
documentation.html  dokumentacja po angielsku (+ PDF)
INSTRUCTIONS.md     instrukcja po angielsku
```

Pliki do druku 3D: wkrótce na MakerWorld.
