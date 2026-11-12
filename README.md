# CatFeeder — ESP32-C3 board with built-in OLED

Hardware and project reference so we never have to re-google the device.
Every fact is tagged by how well it is established:

- ✅ **Confirmed** — measured on the device, or read from this project's code/config
- 📦 **Seller listing** — from the AliExpress description, not verified
- ❓ **Assumed** — inherited from the code or a board variant, not proven on hardware

---

## 1. The board

Confirmed ✅ (read from `platformio.ini`, the board definition and the toolchain):

- Target used by the code: `esp32-c3-devkitm-1` (Espressif ESP32-C3-DevKitM-1)
- MCU: ESP32-C3, **RISC-V** (`rv32`) — not Xtensa; toolchain `toolchain-riscv32-esp`,
  linker script `esp32c3_out.ld`
- Clock frequency: 160 MHz (`f_cpu = 160000000L`)
- Flash: 4 MB total (`maximum_size = 4194304`); the default **app partition** is
  1 310 720 bytes — that is the number that matters when the build gets bigger
- RAM: 327 680 bytes
- Flash mode `qio`, `f_flash` = 80 MHz
- WiFi + Bluetooth present in the core (`CONFIG_ESP_WIFI_ENABLED=y`)
- USB-CDC boot mode: enabled and **tested working** on this board
- RTC: internal RC clock (`CONFIG_ESP32C3_RTC_CLK_SRC_INT_RC=y`), 576 cal cycles
- ESP32-C3 revision range in the prebuilt core libs: rev 3 … 199
- GPIO lines are 0–21 (Espressif hardware reference); several are boot-reserved,
  so do not repurpose pins blindly. GPIO 5 and 6 are taken by the OLED (see §3)

From the seller listing 📦 (not verified):

- Brand: YOINNOVATI — product: "ESP32-C3 0.42-inch OLED Development Board"
- MCU variant: ESP32-C3FN4 / FH4 — the letter suffix has **never** been read off
  the chip, treat it as unconfirmed
- "Built-in flash: 4M" and "Clock frequency: 160MHz" — these do match the
  PlatformIO board definition above
- 2.4G WiFi & Bluetooth 5, onboard ceramic antenna
- Interface channels: UART 2, ADC 5, SPI 1, GPIO/PWM 11 — this is the ESP32-C3
  assignable-function "channel" concept; unused by this project and never probed
- "Chuan C interface: 1 channel" — a mistranslation from the Chinese listing,
  ignore it

---

## 2. Software stack (all ✅ read from this environment)

- PlatformIO Core v6 → commands are `pio run …`; the old `platformio build`
  subcommand no longer exists
- Platform package: `espressif32` 8.0.0
- Framework: Arduino, core release **3.3.12**
- ESP-IDF baked into the prebuilt core libs: **5.5.5**
- Only external library: `olikraus/U8g2` **2.36.18**
- lwIP SNTP client: enabled, max 3 servers, internal re-sync every 3 h
  (`CONFIG_LWIP_SNTP_UPDATE_DELAY=10800000`)
- USB build flags — keep them, they are what makes upload + Serial Monitor work:
  `-D ARDUINO_USB_MODE=1`, `-D ARDUINO_USB_CDC_ON_BOOT=1`
- Serial Monitor speed: 115200 (`monitor_speed` + `Serial.begin(115200)`)
- Upload speed default: 460800 baud

Last clean build — 0 errors, 0 warnings:

- RAM: 39 120 / 327 680 B (11.9 %)
- Flash: 1 052 013 / 1 310 720 B (80.3 %) of the default app partition

---

## 3. Built-in OLED display

Confirmed ✅:

- Interface: **I2C**
- SDA: **GPIO 5**
- SCL: **GPIO 6**
- I2C address: **0x3D** — detected with a real I2C scanner. Never document 0x3C
  (that is the generic SSD1306 default and is wrong for this board). U8g2 is
  given the shifted value `0x3D * 2`.

Not verified ❓:

- Resolution **72 × 40 px** — assumed, because the code compiles the U8g2 class
  `U8G2_SSD1306_72X40_ER_F_HW_I2C` and the earlier "OLED OK!" test drew inside
  those bounds. No independent probe yet.
- Controller **SSD1306** — assumed from the same class plus the I2C behaviour.
- Diagonal **0.42 inch** — 📦 seller listing only, never measured.

Best available evidence (library, not a hardware probe): U8g2's driver
`u8x8_d_ssd1306_72x40_er` is commented as an "EastRising 0.41 OLED / 0.42-inch
72x40 I2C/SPI SSD1306" panel. That supports the SSD1306 + 72×40 assumption but
does not prove it.

Driver facts read from the U8g2 source (true about the library; still ❓ about
our panel):

- `pixel_width / pixel_height` = 72 / 40
- `default_x_offset` = **28** — the 72 px window sits at column 28 inside the
  SSD1306's 128 px addressing; this is what the `ER` suffix means
- Frame buffer `u8g2_m_9_5_f` = 9 × 5 tiles × 8 = **360 bytes** of RAM
- I2C adapter `u8x8_cad_ssd13xx_fast_i2c`, transport `u8x8_byte_arduino_hw_i2c`
- Init sequence sent once by `display.begin()`:

  ```
  0xAE off · 0xD5/0x80 clock+osc · 0xA8/0x27 multiplex (1/40 duty) · 0xD3/0x00 offset
  0xAD/0x30 internal IREF (needed on this panel, u8g2 issue #1047) · 0x8D/0x14 charge pump
  0x40 start line · 0xA6 normal · 0xA4 RAM→display · 0x20/0x00 horizontal addressing
  0xA1 segment remap · 0xC8 scan dir · 0xDA/0x12 COM config · 0x81/0xAF contrast
  0xD9/0x22 pre-charge · 0xDB/0x20 vcomh · 0x2E no-scroll
  ```

- Power save: `0xAF` on / `0xAE` off; flip modes `0xA1/0xC8` vs `0xA0/0xC0`

Code — do not change the pins or the address:

```cpp
U8G2_SSD1306_72X40_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, 6, 5); // SCL=6, SDA=5
display.setI2CAddress(0x3D * 2);   // must run before begin()
display.begin();
```

Pixel budget currently in use (72 px wide, 40 px tall):

- Date line — `u8g2_font_6x10_tf`, `12.11.2026` = 60 px, baseline y = 16
- Time line — `u8g2_font_6x10_tf`, `15:45:32` = 48 px, baseline y = 29
- Status line — `u8g2_font_5x7_tf`, `Sync time...` = 60 px, baseline y = 20
- Status strings in use: `WiFi...`, `Sync time...`, `No WiFi`
- Centring is automatic via `getStrWidth()`; text wider than 72 px is clamped to
  x = 0 and truncated by the 12-byte line buffers — check `getStrWidth()` before
  adding any longer message

How to re-verify the display if it is ever in doubt:

1. Run an I2C scanner over GPIO 5/6 — it must report `0x3D` (already ✅)
2. Draw a frame at `(0,0,72,40)` and a pixel at `(71,39)`; if the corner lights
   up, 72×40 is confirmed
3. There is no "query controller" command on these chips — the practical test is
   the I2C address plus which U8g2 device class actually renders

---

## 4. Project layout

- `platformio.ini` — board, USB-CDC flags, `monitor_speed` (don't churn it)
- `include/wifi_config.h` — **real credentials**, git-ignored, created by the user
- `include/wifi_config.example.h` — tracked template showing the format
- `src/main.cpp` — credential include guard, module objects, non-blocking `loop()`
- `src/wifi_manager.{h,cpp}` — WiFi state machine: CONNECTING / CONNECTED / LOST
- `src/time_manager.{h,cpp}` — NTP sync + cached local `tm` (Europe/Kyiv)
- `src/display_manager.{h,cpp}` — owns the U8g2 object, centred rendering,
  redraw only on change

Modules:

- **WiFiManager** — `begin(ssid, pwd)`, `update()`, `state()`, `connected()`
  - `begin()` only starts the async handshake
  - `update()` polls once per loop pass, prints the assigned IP on success,
    retries after a timeout/hard failure, re-arms 10 s after a lost link
- **TimeManager** — `begin()`, `update(wifiConnected)`, `hasValidTime()`, `localTime()`
  - arms lwIP SNTP via `configTzTime()` only when WiFi is up
  - success = the system clock reports year ≥ 2020 (never compile-time `__DATE__`)
  - retries after 20 s, re-arms every 15 min, reads the clock at most once a second
- **DisplayManager** — `begin()`, `showStatus(text)`, `showClock(tm)`
  - redraws only when the rendered strings actually change → ~1 redraw/s while
    the clock runs, 0 redraws for a static status message

Nothing blocks: no `delay()`, no `waitForConnectResult()`, no
`while(!WiFi.isConnected())`. All periodic work runs on `millis()`.

Tunables:

- `CONNECT_TIMEOUT_MS` = 30 s, `RECONNECT_DELAY_MS` = 10 s — `src/wifi_manager.cpp`
- `TIMEZONE` = `EET-2EEST,M3.5.0/3,M10.5.0/4`, `NTP_SERVER_1` = `pool.ntp.org`,
  `NTP_SERVER_2` = `time.google.com` — `src/time_manager.cpp`
- `SYNC_TIMEOUT_MS` = 20 s, `RESYNC_INTERVAL_MS` = 15 min,
  `CLOCK_REFRESH_MS` = 1000 ms — `src/time_manager.cpp`
- `STATUS_Y` = 20, `CLOCK_LINE1_Y` = 16, `CLOCK_LINE2_Y` = 29 — `src/display_manager.cpp`

Timezone note: the POSIX string above is the Europe/Kyiv rule checked against
`zdump` — DST starts the last Sunday of March at 03:00 local standard (→ 04:00
EEST) and ends the last Sunday of October at 04:00 local DST (→ 03:00 EET). It is
correct from 2011 to present, so winter/summer time switches itself; never
hardcode a fixed UTC offset.

---

## 5. Build, upload, monitor

```bash
cd /home/snaildog/Documents/Experiments/Devices/CatFeeder/Firmware/CatFeeder

pio run -t build   -e esp32-c3-devkitm-1   # compile + size report
pio run -t clean   -e esp32-c3-devkitm-1   # wipe .pio/build
pio run            -e esp32-c3-devkitm-1   # plain build
pio run -t upload  -e esp32-c3-devkitm-1   # flash over USB CDC
pio device monitor -e esp32-c3-devkitm-1   # Serial Monitor @115200
pio device list                            # find the device node
```

- If `pio` is not on `PATH`, use `~/.platformio/penv/bin/pio …`
- `include/wifi_config.h` must exist before building; otherwise the build fails
  on purpose with:

  ```
  error: #error "Missing include/wifi_config.h: copy include/wifi_config.example.h to include/wifi_config.h and define WIFI_SSID and WIFI_PASSWORD."
  ```

- Expected Serial Monitor trace:

  ```
  ESP32-C3 CatFeeder: startup
  Display: OLED init (I2C 0x3D, SCL=6, SDA=5)
  WiFi: connection attempt 1
  WiFi: connected, assigned IP: 192.168.x.x
  Time: NTP sync attempt 1 (pool.ntp.org, time.google.com)
  Time: NTP sync OK, local time = 12.11.2026 15:45:32 (DST=0)
  ESP32-C3 CatFeeder: entering main loop
  ```

- OLED sequence: `WiFi...` → `Sync time...` → `12.11.2026` / `15:45:32`
- Kill the network and you should see `No WiFi` plus reconnect attempts every 10 s

---

## 6. Gotchas

- ESP32-C3 is RISC-V — no Xtensa, no AVP, no S3-style SRAM. Don't pull in
  ESP32-S3 / ESP32-C6 specific libraries.
- Keep the USB-CDC `build_flags`; they are what makes upload + Serial Monitor work.
- Keep GPIO 5/6 and I2C address `0x3D`; `setI2CAddress()` must run **before**
  `display.begin()`.
- U8g2 fonts dominate flash: each extra `_tf` font ≈ 1.6 KB
  (`u8g2_font_5x7_tf` is 1612 bytes). ~250 KB margin left in the app partition.
- `WiFi.begin()` is non-blocking in core 3.3.12 (it only calls
  `esp_wifi_connect()`); polling `WiFi.isConnected()` is the right pattern.
- lwIP's SNTP client already re-polls on its own every 3 h; the app's 15-minute
  re-arm is what gives faster, visible re-sync and retry logging.
- Credentials are compile-time macros in a git-ignored header — never move them
  into `main.cpp`, never commit `include/wifi_config.h`.

## 7. Proven vs assumed

- ✅ Builds and links for `esp32-c3-devkitm-1`, 0 warnings, RAM 11.9 %,
  Flash 80.3 % of the default app partition
- ✅ USB-CDC upload + 115200 Serial Monitor (tested earlier on this board)
- ✅ OLED over I2C, GPIO 5 = SDA, GPIO 6 = SCL, address **0x3D** (I2C scanner)
- ❓ OLED 72 × 40 and controller SSD1306 — library evidence only, not probed
- 📦 0.42-inch diagonal, ESP32-C3FN4/FH4, ceramic antenna, interface-channel
  counts (4 MB flash and 160 MHz do match the board definition)
- ❓ Firmware flashed and observed running — not done in this session (no ESP
  device was attached)


