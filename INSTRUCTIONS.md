# Spool Film Processor – step-by-step guide

**English** · [Polski](docs/pl/INSTRUKCJA.md)

Spool Film Processor – a film developing processor built on an **ESP32-C3 with a 2.4" touch screen (msalamon EVB-00055)**.

```
spool-film-processor\
├── SpoolFilmProcessor\          <- firmware (Arduino sketch)
│   ├── SpoolFilmProcessor.ino   setup() / loop(), serial test commands
│   ├── config.h                 ALL pins and constants (change hardware here)
│   ├── compensation.h           temperature / chemistry-reuse compensation, tank thermal model
│   ├── programs.h               factory programs, settings, flash storage
│   ├── motor.h                  TMC2209 over UART (1 pin) + agitation modes
│   ├── thermo.h                 DS18B20
│   ├── touch.h                  touch controller auto-detect + calibration
│   ├── bootio.h                 BOOT button (START) + speaker melodies
│   ├── ota.h                    WiFi update
│   ├── display.h                ST7789 display setup
│   ├── spool_logo.h             Spool logo bitmaps
│   └── ui.h                     all screens
├── firmware\                    ready-made .bin files and flashing scripts
├── documentation.html           BOM, schematics, wiring, assembly (open in a browser)
└── INSTRUCTIONS.md              this file
```

---

## 1. What the device does

| Feature | Details |
|---|---|
| Factory programs (B&W, C-41, E-6, ECN-2, RA-4) | 8 programs: B&W generic, DF96, C-41 Tetenal, C-41 Cinestill, C-41 ADOX C-TEC 41, E-6, ECN-2, RA-4 |
| Custom programs | 4 slots W1–W4; **COPY** duplicates any program |
| Step editing: name, time, water changes, agitation, time control | Yes, all on the touch screen |
| Time compensation for temperature during the process | Yes, integrated in real time (see section 7) |
| Chemistry reuse | Rolls used per program; developer and blix times grow per manufacturer tables |
| MON: temperature range monitor | Yes, red thermometer panel when out of range |
| Agitation ROLL / STICK / STAND / none | Yes (default 60 rpm, 15/14 s etc.) |
| Motor keeps running 15 s after the timer ends | Yes, then the next step |
| Automatic B&W developer time reduction (15%) | Yes (Settings → B&W time reduction) |
| Probe not in chemistry | Tank thermal model + tap-water thermometer with too hot / too cold beeps |
| Sound, brightness, °C/°F, PL/EN, reset of programs/settings | Yes |
| WiFi update | Yes (Settings → WiFi update) |
| Screen rotation 180° | Yes (Settings); touch rotates with the screen |
| **BOOT** button on the board | START / PAUSE / NEXT (works with wet fingers) |

---

## 2. Installing the software on a PC (Windows)

1. **Arduino IDE 2.x**: download from <https://www.arduino.cc/en/software> and install it (accept the driver installation).
2. Arduino IDE → **File → Preferences** → *Additional boards manager URLs* → paste:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   → OK.
3. **Tools → Board → Boards Manager** → search `esp32` → **esp32 by Espressif Systems** → install version **3.x** (tested with 3.3.12).
4. **Tools → Manage Libraries** → install:
   - **LovyanGFX** (lovyan03) – tested 1.2.32
   - **OneWire** (Paul Stoffregen) – tested 2.3.8
   - **DallasTemperature** (Miles Burton) – tested 4.0.6. If asked about dependencies, choose *Install all*.
5. **File → Open** → `spool-film-processor\SpoolFilmProcessor\SpoolFilmProcessor.ino`
   (all `.h` files open as tabs).

## 3. Board settings in Arduino IDE

**Tools** menu:

| Option | Value |
|---|---|
| Board | **ESP32C3 Dev Module** |
| USB CDC On Boot | **Enabled** ← required, otherwise compilation stops with an error |
| CPU Frequency | 160 MHz |
| Flash Size | 4MB |
| Partition Scheme | **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** |
| Upload Speed | 921600 |
| Port | the COMx that appears after plugging in the board |

> Why *USB CDC On Boot = Enabled*? Debug messages then go over USB-C, so GPIO20/21 (RXO/TXO) stay free for the probe and the motor.

## 4. Flashing

1. Connect **the ESP32 board** to the PC with a USB-C cable. The cable must carry data, not just power.
2. Pick the port in **Tools → Port**. The board uses the ESP32-C3's built-in USB, so no CH340 driver is needed.
3. Click **Upload (→)**.
4. If uploading does not start (`Failed to connect` / no port):
   hold **BOOT**, briefly press **RESET** (复位), release **BOOT** and upload again. Press **RESET** after flashing.
5. Open **Tools → Serial Monitor** at **115200**. You should see something like:
   ```
   [FILM PROCESSOR] start, FW 2.1
   [TOUCH] znaleziono: adres 0x15, SDA=GPIO4, SCL=GPIO5, typ=CST816/FT6x36
   [TEMP] DS18B20 znaleziony: 28FF....
   ```
   (serial log messages are in Polish).

No Arduino IDE? Use the ready-made files in `firmware\` – see the README.

Later updates can be done without a cable: **Sketch → Export Compiled Binary**, then on the device **Settings → WiFi update**. Connect your phone/PC to the `FilmProcessor` network (password `film1234`), open `http://192.168.4.1` and choose `SpoolFilmProcessor.ino.bin` from the `build` folder.

---

## 5. Testing – in stages (always simplest first)

### Stage A – board only (powered from the PC's USB)
1. After flashing the **SPOOL** logo appears, then the main menu.
2. **Touch:** the firmware searches for the touch controller on GPIO1–5 by itself.
   - If it shows *“No touch detected”*, touch the screen – the CST816 sleeps and wakes up on touch.
   - The pins found are stored, so the next boot is faster.
3. **Calibration** (automatic on first boot): touch the exact centre of 3 crosses.
   Then a test screen: touch it, green dots should appear under your finger. Press **DONE** (or BOOT). Without confirmation the calibration restarts after 20 s.
   - Repeat calibration from *Settings → Touch calibration*, or **touch the screen while the logo is shown** after power-on.
4. **Screen rotation:** *Settings → Rotate screen 180°*. Touch rotates automatically, no recalibration needed.
5. Browse the menu, open *C-41 Tetenal*, edit a step time. Exit, power off, power on and check that the change was saved.

### Stage B – DS18B20 probe
1. Wire it per the schematic: red → 3.3V, black → GND, yellow → RXO (GPIO20), **4.7 kΩ between yellow and 3.3V**.
2. *Menu → Thermometer*: the temperature should appear. `--.-` means no probe or no 4.7 kΩ resistor.
3. Put the probe in water next to a good kitchen thermometer and enter the difference in *Settings → Sensor offset*.

### Stage C – power (without the ESP and without the motor!)
1. Charger module + battery: check pad polarity and 5 V at the output.
2. **XL6009**: feed it 5 V and, with no load, set **12.0 V** at the output with the potentiometer. Only then connect it to the TMC2209.

### Stage D – TMC2209 and motor
1. Wire the TMC2209 per the schematic: VIO → 3.3V, VM → 12V + 1000 µF capacitor, EN / MS1 / MS2 / STEP / DIR → GND, **PDN** → 1 kΩ → TXO. USART and CLK stay unconnected.
2. Connect and disconnect the motor **only with power off**.
3. *Settings → Motor test → START*. The motor should turn smoothly and quietly (stealthChop).
   - −/+ change speed and current. Start at **600–700 mA**.
   - Motor stalls under load (full tank)? Increase current by 100 mA.
   - Motor too hot to touch (> 60 °C)? Reduce current.
   - Motor only buzzes and shakes? One coil is wired wrong – check the pairs with a multimeter (~3 Ω).
   - Wrong direction? `config.h`: `MOTOR_INVERT true`.
4. Communication with the TMC2209 is **one-way** (the ESP only sends). If the motor does not react at all, check in order: 12 V on VM, 3.3 V on VIO, the 1 kΩ resistor, that the wire goes to the PDN pin (not USART/RX), and that EN / MS1 / MS2 are on GND.

### Stage E – dry run with water
1. Fill the tank with water at about 30 °C.
2. Start *C-41 Tetenal*: **START** on the HEAT step, then **SKIP** (tap twice).
3. DEV step: nominal 3:15 at 38 °C. At 30 °C the program should show about **+4:45** (≈ 8:00) and a multiplier of **x2.46**.
4. Add warm water – the remaining time should get shorter. That is real-time compensation.

---

## 6. Operation

1. After power-on the device opens the **last used program**. Otherwise: **Main menu → Factory programs → process type → program.**
2. Program screen: list of steps `NAME  TIME  AGITATION  CONTROL`.
   Tap a step to edit it. **RESET** restores the factory program; custom programs have **COPY** there.
3. **START** → setup screen (upright / lying, probe in chemistry, volume, pre-wash temperature) → **NEXT** → *“POUR IN: Developer”*. Pour the chemistry and press **START** (or BOOT).
4. During the step you see: remaining time, time added by compensation (`+0:42`), temperature (grey = in range, red = out of range) and the time multiplier.
   **PAUSE**, **SKIP** and **STOP** (the last two need a double tap).
5. When the timer ends: melody, *“DRAIN”*, the motor runs another 15 s, then the next step. **NEXT** skips the 15 s.
6. After the last step *“DONE!”* shows the total time; enter the rolls in the tank and press SAVE & FINISH to store chemistry usage.

**Agitation (Settings):**
- ROLL – continuous rotation, T1 s clockwise, T2 s counter-clockwise (default 60 rpm, 15/14 s)
- STICK – T1 s continuously, then T2 s every minute (60 rpm, 60/10 s)
- STAND – T2 s at the start, then T3 s of rotation every T1 minutes (60 rpm, 30 min / 60 s / 30 s)

Measured compensation is disabled for STICK and STAND because the upright probe does not touch the chemistry; use the “probe not in chemistry” mode (tank thermal model) instead.

---

## 7. Temperature compensation algorithm

File: `compensation.h`.

**Model.** Development speed grows exponentially with temperature (Arrhenius law).
Each chemistry has a factor curve:

```
F(T) = time_at_T / time_at_T_nominal        F(T_nom) = 1.00
```

stored as a few points from manufacturer tables. Between points the interpolation is **linear in the logarithm**,
which gives exactly an exponential curve:

```
ln F(T) = ln F1 + (ln F2 − ln F1) · (T − T1) / (T2 − T1)
```

Outside the table the curve is extended by at most 5 °C and F is clamped to 0.25–4.0.

**Real-time integration.** The chemistry cools during a step, so the time is not computed once at the start.
Every ~15 ms the firmware adds “development progress”:

```
progress += Δt / (t_nom · F(T_now) · reuse)     step ends at progress ≥ 1
remaining  = (1 − progress) · t_nom · F(T_now) · reuse
added time = (elapsed + remaining) − t_nom      → shown as “+0:42”
```

**Example (C-41 Tetenal, 3:15 at 38.0 °C):**

| Temperature | F | DEV step time |
|---|---|---|
| 39 °C | 0.92 | 2:59 |
| 38 °C | 1.00 | 3:15 |
| 36 °C | 1.29 | 4:12 |
| 34 °C | 1.62 | 5:16 |
| 30 °C | 2.46 | 8:00 |

If half the step runs at 36 °C and the other half at 35 °C, the step takes 2:06 + 2:21 = **4:27**.

**Curves in the firmware** (approximations from public tables – **check your chemistry's datasheet**):

| Curve | T nom | “In range” | Source |
|---|---|---|---|
| BW – B&W generic | 20 °C | 18–24 °C | Ilford chart (≈ −9.5%/°C) |
| DF96 | 26.7 °C | 21–30 °C | Cinestill DF96 |
| C41T – C-41 Tetenal/Bellini | 38 °C | 30–40 °C | Tetenal Colortec |
| C41C – C-41 Cinestill/Arista | 39 °C | 24–41 °C | Cinestill Cs41 (102°F 3:30, 80°F 8:00) |
| ADOX – C-TEC 41 | 38 °C | 24–39 °C | ADOX datasheet |
| BLIX – C-41/E-6 | 38 °C | 30–40 °C | less sensitive |
| E6FD – E-6 first developer | 38 °C | 36–40 °C | very sensitive |
| E6CD – E-6 colour developer | 38 °C | 35–40 °C | |
| ECN2 | 41.1 °C | 30–43 °C | Kodak SM-2 |
| RA4 | 35 °C | 25–38 °C | |

**Adding or fixing a curve for your chemistry:** take a few (temperature, time) pairs from the manufacturer table and divide each time by the time at the nominal temperature. Enter the points as `{temp×10, F×1000}`, for example:

```cpp
// my developer: 20°C 8:00, 24°C 5:30 → F(24) = 330/480 = 0.688
static const CurvePt C_MY[] = { {180, 1220}, {200, 1000}, {240, 688} };
// and in the CURVES[] table:
{"My developer", "MY", 200, 180, 240, C_MY, N_(C_MY)},
```

The new curve appears in the step editor under “Time control”.

---

## 8. Power and switch

- **The ESP32 cannot switch itself off completely.** It can only put the CPU to sleep; the screen, TMC2209 and XL6009 would keep drawing current. That is why there is **switch SW1**.
- Put SW1 **between the charger module's 5 V output and the rest of the circuit**, not between the battery and the charger. Then:
  - the switched-off device draws nothing from the battery,
  - USB-C charging works with the device switched off.
- These modules can switch their output off by themselves at very low load (< ~50 mA for ~30 s). If the device turns off by itself in the menu, wire a button between the **K** pad and GND (a short press turns the output on) or raise the screen brightness.
- Battery level cannot be measured (no free pin) – the charger module's LEDs show it.

## 9. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `#error ... USB CDC On Boot` when compiling | Tools → USB CDC On Boot → **Enabled** |
| `#error ... 3.x` | update *esp32 by Espressif* to 3.x in Boards Manager |
| `Sketch too big` | Tools → Partition Scheme → **Minimal SPIFFS (1.9MB APP with OTA)** |
| No COM port | another (data) cable, BOOT+RESET mode (section 4) |
| Black screen | check the backlight (GPIO0). Inverted colours: `TFT_INVERT false` in `config.h` |
| “No touch detected” | touch the screen; if still nothing, check the Serial Monitor log |
| Touch lands off target | Settings → Touch calibration, or touch the screen during the logo |
| Temperature `--.-` | missing 4.7 kΩ resistor, swapped wires, wrong pin (must be RXO) |
| Motor does not move | 12 V on VM? 3.3 V on VIO? PDN (not USART!) via 1 kΩ to TXO? EN, MS1, MS2 on GND? Common ground? |
| Motor shakes / buzzes | coils wired wrong – check the pairs with a multimeter |
| Turns off in the menu | see section 8 (charger module auto-off) |

---

## 10. Hardware constants (`config.h`)

| Constant | Meaning |
|---|---|
| `PIN_TOUCH_SDA / SCL` | fixed touch pins (instead of auto-detect) |
| `BUZZER_ENABLED`, `BUZZER_PASSIVE` | speaker on GPIO9 through the LM386 amplifier (documentation.html, section 3) |
| `TMC_RSENSE` | sense resistors on the TMC2209 module (0.10 on the Tenstar V985) |
| `GEAR_RATIO` | if there is a gear between the motor and the tank |
| `MOTOR_INVERT` | reverse direction |
| `OVERRUN_SEC` | seconds the motor keeps running after the timer ends (default 15) |
| `OTA_SSID / OTA_PASS` | name and password of the update network |
