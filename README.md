# CatFeeder

Automatic cat feeder on an ESP32-C3 board with a built-in OLED display.

- The board connects to WiFi and sets its clock from an NTP server.
- The OLED shows the current time, or a status message while something is happening.
- Meals are scheduled at fixed times of day. The motor that dispenses food is not implemented yet.

## Hardware

- Board: `esp32-c3-devkitm-1` (Espressif ESP32-C3-DevKitM-1), sold as "ESP32-C3 0.42-inch OLED Development Board" by YOINNOVATI
- MCU: ESP32-C3FN4, RISC-V (`rv32`), not Xtensa — toolchain `toolchain-riscv32-esp`, linker script `esp32c3_out.ld`
- Clock frequency: 160 MHz
- Flash: 4 MB total, default app partition 1 310 720 bytes
- RAM: 327 680 bytes
- Flash mode `qio`, `f_flash` = 80 MHz
- WiFi + Bluetooth 5, onboard ceramic antenna
- OLED panel: 0.42 inch, 72 × 40 px, SSD1306
- GPIO lines 0–21, several are boot-reserved; GPIO 5 and 6 are used by the OLED
- USB-CDC boot mode works: upload at 460800 baud, Serial Monitor at 115200

## Software stack

- PlatformIO Core 6 — commands are `pio run …`, the old `platformio build` subcommand no longer exists
- Platform `espressif32` 8.0.0, framework Arduino, Arduino core 3.3.12
- ESP-IDF 5.5.5 baked into the prebuilt core libraries
- Only external library: `olikraus/U8g2` 2.36.18
- lwIP SNTP client: enabled, max 3 servers, re-syncs internally every 3 hours
- USB-CDC build flags, keep them, they are what makes upload and Serial Monitor work:
  - `-D ARDUINO_USB_MODE=1`
  - `-D ARDUINO_USB_CDC_ON_BOOT=1`

Last clean build: 0 errors, 0 warnings

- RAM: 39 152 / 327 680 bytes (11.9 %)
- Flash: 1 053 599 / 1 310 720 bytes (80.4 %) of the default app partition

## Display

The OLED talks to the board over I2C.

- SDA: GPIO 5
- SCL: GPIO 6
- I2C address: `0x3D` — U8g2 takes the shifted value `0x3D * 2`
- `setI2CAddress()` must run before `display.begin()`

```cpp
U8G2_SSD1306_72X40_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, 6, 5); // SCL=6, SDA=5
display.setI2CAddress(0x3D * 2);
display.begin();
```

`display.begin()` sends the full SSD1306 init sequence, there is nothing to configure by hand.

The panel is 72 × 40 px. What is drawn on it:

- Date line — `u8g2_font_6x10_tf`, `12.11.2026` = 60 px, baseline y = 16
- Time line — `u8g2_font_6x10_tf`, `15:45:32` = 48 px, baseline y = 29
- Status line — `u8g2_font_5x7_tf`, `Sync time...` = 60 px, baseline y = 20
- Status strings in use: `WiFi...`, `Sync time...`, `No WiFi`, `Feeding`

Text is centred with `getStrWidth()`. Anything wider than 72 px is clamped to x = 0 and cut off by the 12-byte line buffers, so check `getStrWidth()` before adding a longer message.

Fonts dominate the flash budget: each extra `_tf` font costs about 1.6 KB (`u8g2_font_5x7_tf` is 1612 bytes), and about 250 KB is left in the app partition.

If the panel size is ever in doubt, run an I2C scanner over GPIO 5/6 — it must report `0x3D`.

## Project layout

- `platformio.ini` — board, USB-CDC flags, `monitor_speed`
- `include/wifi_config.h` — real credentials, git-ignored, created by the user
- `include/wifi_config.example.h` — tracked template showing the format
- `src/main.cpp` — credential include guard, module objects, non-blocking `loop()`
- `src/wifi_manager.{h,cpp}` — WiFi state machine: CONNECTING / CONNECTED / LOST
- `src/time_manager.{h,cpp}` — NTP sync and cached local `tm`
- `src/display_manager.{h,cpp}` — owns the U8g2 object, centred rendering, redraw only on change
- `src/motor_manager.{h,cpp}` — motor seam, currently a mock
- `src/feeder_manager.{h,cpp}` — time-of-day schedule and IDLE/ROTATING state machine

### WiFiManager

`begin(ssid, pwd)`, `update()`, `state()`, `connected()`

- `begin()` only starts the async handshake
- `update()` polls once per loop pass, prints the assigned IP on success, retries after a timeout or hard failure, re-arms 10 s after a lost link

### TimeManager

`begin()`, `update(wifiConnected)`, `hasValidTime()`, `localTime()`

- arms lwIP SNTP through `configTzTime()` only when WiFi is up
- a sync counts as successful when the system clock reports a year of 2020 or later, never the compile-time `__DATE__`
- retries after 20 s, re-arms every 15 min, reads the clock at most once a second

### DisplayManager

`begin()`, `showStatus(text)`, `showClock(tm)`

- redraws only when the rendered strings actually change: about one redraw per second while the clock runs, none for a static status message

### MotorManager

`begin()`, `update()`, `startRotation()`, `stopRotation()`, `state()` — mock today, see the Motor section

### FeederManager

`begin(motor&)`, `update(timeValid, clock)`, `state()`, `feeding()`

- `begin()` injects the motor by reference, no virtuals and no heap
- a due slot starts the rotation and the duration is measured with `millis()` on later loop passes, nothing blocks
- one meal per slot per calendar day, the flags reset when `tm_yday` or `tm_year` change
- the first valid clock marks slots that are already in the past as done without feeding, so booting at 15:45 does not dump food for the 06:30 slot
- a clock that jumps forward past a slot still feeds it, late and logged; a clock that jumps back never re-feeds a slot that already fired

Nothing blocks: no `delay()`, no `waitForConnectResult()`, no `while(!WiFi.isConnected())`. All periodic work runs on `millis()`.

### Tunables

- `CONNECT_TIMEOUT_MS` = 30 s, `RECONNECT_DELAY_MS` = 10 s — `src/wifi_manager.cpp`
- `TIMEZONE` = `EET-2EEST,M3.5.0/3,M10.5.0/4`, `NTP_SERVER_1` = `pool.ntp.org`, `NTP_SERVER_2` = `time.google.com` — `src/time_manager.cpp`
- `SYNC_TIMEOUT_MS` = 20 s, `RESYNC_INTERVAL_MS` = 15 min, `CLOCK_REFRESH_MS` = 1000 ms — `src/time_manager.cpp`
- `STATUS_Y` = 20, `CLOCK_LINE1_Y` = 16, `CLOCK_LINE2_Y` = 29 — `src/display_manager.cpp`
- `SCHEDULE` = 06:30 / 15:00 / 22:00, `ROTATION_MS` = 5 s — `src/feeder_manager.cpp`; `MAX_FEED_SLOTS` = 3 is in `src/feeder_manager.h`, and a `static_assert` keeps the table and the array in step when the schedule grows

Schedule times are local minutes of the day (`tm_hour * 60 + tm_min`), never a fixed UTC offset. `TIMEZONE` is the Europe/Kyiv rule: DST starts the last Sunday of March at 03:00 local standard and ends the last Sunday of October at 04:00 local DST. The rule has been unchanged since 2011, so 06:30 stays 06:30 wall-clock through both switches.

## Motor — not implemented yet

`src/motor_manager.cpp` is a mock: `startRotation()` and `stopRotation()` only log a line, no pin is configured, no motor library is used.

The split is there so that wiring a real motor touches one file:

```
FeederManager  →  policy only: when to feed, how long to run, IDLE/ROTATING
      │            no pins, no libraries, testable today
      │ holds MotorManager&
      ▼
MotorManager   →  the seam: startRotation() / stopRotation() / configurePins()
                  today = empty bodies + one log line each
                  later = pin numbers, pinType, attach(), direction, jam detect
```

`MotorManager::update()` is already called from `loop()` even though the mock leaves it empty. Stepper-style motor libraries need a tick per loop pass, and having the hook now means `main.cpp` does not change when the real motor arrives.

When the hardware is known, fill in only these in `src/motor_manager.cpp`:

- `configurePins()` — motor/controller pin numbers, `pinType`, `attach()` if a stepper library is used, rotation direction and speed
- `startRotation()` / `stopRotation()` — the real actuation
- `update()` — the per-loop tick the library needs
- `MotorState::NOT_CONFIGURED` — keep it until the pins are actually wired

Pins:

- GPIO 5 (SDA) and 6 (SCL) are taken by the OLED, do not repurpose them
- GPIO numbers on ESP32-C3 run 0–21; check which of the rest are boot-reserved on this board before choosing
- Motor model, controller model and the physical pins are still unknown

`lib_deps` still lists only `olikraus/U8g2`. A stepper library gets added there when the real motor arrives.

## Build, upload, monitor

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
- `include/wifi_config.h` must exist before building, otherwise the build fails on purpose:

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
  Motor: mock backend, no pins configured
  Feeder: 3 slots, rotation 5000 ms
  ESP32-C3 CatFeeder: entering main loop
  ```

- OLED sequence: `WiFi...` → `Sync time...` → `12.11.2026` / `15:45:32`
- Kill the network and you should see `No WiFi` plus reconnect attempts every 10 s
- At a scheduled minute the trace continues (the mock only logs):

  ```
  Feeder: slot 1 due at 15:00
  Motor: startRotation (mock)
  Motor: stopRotation (mock)
  ```

  and the OLED shows `Feeding` for those 5 s instead of the clock
- `Feeder: slot N fired M min late` means the clock jumped forward past that slot, the meal still happens

## Notes

- ESP32-C3 is RISC-V — no Xtensa, no AVP, no S3-style SRAM. Don't pull in ESP32-S3 or ESP32-C6 specific libraries.
- `WiFi.begin()` is non-blocking in Arduino core 3.3.12 (it only calls `esp_wifi_connect()`); polling `WiFi.isConnected()` is the right pattern.
- lwIP's SNTP client re-polls on its own every 3 hours; the app's 15-minute re-arm is what gives faster, visible re-sync and retry logging.
- Credentials are compile-time macros in the git-ignored `include/wifi_config.h` — never move them into `main.cpp`, never commit that file.
- Never compare `millis()` against an absolute value; use `millis() - start >= …` so the wrap at 2^32 is safe.
- Feeding only happens when `hasValidTime()` is true: if NTP never succeeds that day, the cat is not fed. There is no manual override yet.
