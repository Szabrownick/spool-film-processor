// =====================================================================
//  config.h  -  WSZYSTKIE piny i stale sprzetowe w jednym miejscu
//  Plytka: msalamon EVB-00055 "ESP32-C3 z wyswietlaczem TFT 2.4 ST7789"
// =====================================================================
#pragma once

#define FW_VERSION "2.3"
// data kompilacji (np. "Oct 10 2026 14:05") - widoczna na intro i w INFORMACJE
#define FW_BUILD   __DATE__ " " __TIME__
#define TOUCH_MIN_GAP_MS  250   // minimalny odstep miedzy kolejnymi kliknieciami
#define TOUCH_RELEASE_N   3     // tyle kolejnych odczytow "puszczone" (co 15 ms) = puszczenie

// ---------------------------------------------------------------------
//  Wbudowany wyswietlacz (polaczony fabrycznie - NIE ZMIENIAC)
//  Wartosci z przykladowego kodu producenta (EVB-00055.ino)
// ---------------------------------------------------------------------
#define PIN_TFT_SCLK   6
#define PIN_TFT_MOSI   7
#define PIN_TFT_DC     10
#define PIN_TFT_RST    8
#define PIN_TFT_CS     -1      // CS na stale do GND -> SPI MODE 3
#define PIN_TFT_BL     0       // podswietlenie, HIGH = wlaczone (PWM)
#define TFT_INVERT     true    // panel IPS wymaga inwersji kolorow

// ---------------------------------------------------------------------
//  Dotyk pojemnosciowy (I2C, polaczony fabrycznie)
//  Producent nie podaje pinow -> firmware sam je wyszukuje sposrod
//  wolnych GPIO (1,2,3,4,5). Jesli znasz piny, wpisz je zamiast -1.
//  Wykryto na egzemplarzu testowym: CST816 0x15, SDA=GPIO4, SCL=GPIO5.
// ---------------------------------------------------------------------
#define PIN_TOUCH_SDA  -1
#define PIN_TOUCH_SCL  -1

// ---------------------------------------------------------------------
//  Zlacze 6-pin na plytce: 5V, GND, 3.3V, TXO(GPIO21), RXO(GPIO20), GND
//  -> tylko DWA wolne piny: jeden na silnik, jeden na czujnik
// ---------------------------------------------------------------------
#define PIN_TMC_UART   21      // TXO -> rezystor 1k -> PDN_UART sterownika TMC2209
#define PIN_DS18B20    20      // RXO -> DATA czujnika (+ rezystor 4.7k do 3.3V)

// ---------------------------------------------------------------------
//  GPIO9 = przycisk BOOT na plytce.
//  Po starcie programu dziala jako przycisk START / PAUZA / DALEJ
//  (przydatny mokrymi rekami). Na tym samym pinie glosnik:
//  GPIO9 -> C 2.2uF -> LM386 -> C 100uF -> glosnik (schemat.html, rozdz. 3).
// ---------------------------------------------------------------------
#define PIN_BOOT_BTN   9
#define BUZZER_ENABLED true    // false = brak brzeczyka (pin tylko jako przycisk)
#define BUZZER_PASSIVE true    // false = brzeczyk aktywny (z generatorem, np. 3V)
                               // true  = glosniczek z obudowy PC / pasywny / piezo (PWM)
                               // (przez LM386 - kondensator na wejsciu chroni start plytki)

// ---------------------------------------------------------------------
//  Silnik i sterownik TMC2209 (tryb UART, wewnetrzny generator krokow VACTUAL)
// ---------------------------------------------------------------------
#define TMC_ADDR           0       // MS1=GND, MS2=GND -> adres 0
#define TMC_BAUD           115200
#define TMC_RSENSE         0.10f   // rezystory pomiarowe: V985 (Tenstar) = 0.10R, BTT/FYSETC = 0.11R
#define MOTOR_STEPS        200     // JK42HS48-1204: 1.8 st. = 200 krokow/obr
#define GEAR_RATIO         1.0f    // obroty silnika na 1 obrot koreksu (przekladnia)
#define MOTOR_INVERT       false   // true = odwraca kierunek obrotow
#define MOTOR_ACCEL_RPM_S  150.0f  // przyspieszenie [obr/min na sekunde]
#define MOTOR_SOFT_ACCEL_RPM_S 40.0f // lagodny rozruch z miejsca: 0 -> 60 obr/min w 1,5 s
// Zmiany kierunku w ROLL: przy 150 obr/min/s nawrot 60 -> -60 obr/min trwa 0,8 s
// (lagodnie dla mieszadla, sprzegla i spiral; moment bezwladnosci spiral z ciecza
// to ulamek momentu silnika). Odcinek musi byc dluzszy niz nawrot, inaczej silnik
// nie zdazy rozpedzic sie w druga strone.
#define ROLL_START_FLIP    2.0f    // start intensywny: zmiana kierunku co 2 s
#define ROLL_BURST_SEG     1.5f    // burst: jeden odcinek (0,8 s nawrot + 0,7 s obrotow)

// ---------------------------------------------------------------------
//  Zakresy
// ---------------------------------------------------------------------
#define RPM_MIN        10
#define RPM_MAX        90
#define CURRENT_MIN_MA 200
#define CURRENT_MAX_MA 1200        // silnik ma 1.2A znamionowo - nie przekraczac
#define OVERRUN_SEC    15          // silnik pracuje jeszcze 15 s po wyzerowaniu czasu

// ---------------------------------------------------------------------
//  Aktualizacja przez WiFi
// ---------------------------------------------------------------------
#define OTA_SSID       "FilmProcessor"
#define OTA_PASS       "film1234"   // min. 8 znakow

// ---------------------------------------------------------------------
//  Sprawdzenie ustawien Arduino IDE
// ---------------------------------------------------------------------
#include <esp_arduino_version.h>
#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "Wymagany pakiet plytek esp32 (Espressif) w wersji 3.x - zaktualizuj w Boards Manager"
#endif
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || (ARDUINO_USB_CDC_ON_BOOT == 0)
#error "Ustaw: Tools -> USB CDC On Boot -> Enabled (GPIO20/21 sa potrzebne dla silnika i czujnika)"
#endif
