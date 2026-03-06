# Working JC3248W535 LVGL display

I got the cheap and widely available JC3248W535 hardware, but had a hard time to get it to behave well with LVGL. Since it took quite some time (and help from copilot) I am sharing my basic working 'proof of concept' code so you can have a jumpstart ..I hope. 

My purpose is to build a smart lightswitch for wled, hence the project name.

# ⚠️ UPDATE - Switch to ESP-IDF upcomming
I have switched to the ESP-IDF stack for performance reasons in my latest code version that i will push to this codebase soon. 

```
Why Arduino Failed vs. Native ESP-IDF
The Arduino framework adds heavy abstraction layers that slow down execution and force graphical data through tight, unoptimized memory bottlenecks, resulting in low FPS and memory crashes (LoadProhibited). Switching to Native ESP-IDF strips away that overhead, granting direct, high-speed access to the DMA controller and 8MB PSRAM. This unlocks smooth, high-framerate UI performance and absolute point-to-point stability that Arduino simply couldn't provide.

Sure you lose some convenience with this switch but performance is loads and loads better! between ~ 24-50 fps vs. ~ 5-22 fps, depending on how graphical complex your screen is..
I will update this codebase on the short term with the ESP-IDF version and fork this code to its own branch for documentation purposes, or for anyone smarter than me to improve on this.

Using Eez studio for the screens.

Enclosed in the new code is an Eez project. This tool is easy to learn, and offers a way for easy GUI design that incorporates the LVGL library. Any changes you make must be build in Eez studio, and will be encorporated in your latest build from PlatformIO.
```

## ✨ What It Does

- Full display rotation (hardware level, not software)
- Touch input works correctly at all angles
- Interactive grid UI demo (6 colored boxes)
- Bottom info bar with live memory info (Heap + PSRAM)
- Backlight slider on screen (1% to 100%)
- Startup splash using GFX
- Partial LVGL buffer mode (40-line stripe, efficient RAM use)

## ⚠️ On your own...
This code comes as it is. It is not optimized since it is a POC and I am certainly no hardware specialist. I am not planning on active monitoring or maintaining the code, but I might...

The upside? You are free to use it in any way you like ;)

## 🧰 Hardware

- **Board**: JC3248W535 (ESP32-S3)
- **Display**: 320×480 QSPI TFT (AXS15231B_JC3248)
- **Touch**: AXS15231B capacitive (I2C)

### 🔌 Wiring

**Display (QSPI):**
- CS: GPIO 45
- SCK: GPIO 47
- D0: GPIO 21
- D1: GPIO 48
- D2: GPIO 40
- D3: GPIO 39
- BL: GPIO 1 (backlight)

**Touch (I2C):**
- SCL: GPIO 8
- SDA: GPIO 4
- INT: GPIO 3
- Addr: 0x3B

## 🚀 Setup

1. **Install PlatformIO**: `pip install platformio`

2. **Clone and build:**
   ```bash
   git clone <repo>
   cd LightSwitch
   pio run -j 8
   ```

3. **Upload:**
   ```bash
   pio run --target upload --upload-port /dev/ttyACM0
   ```

### ✅ VS Code tasks (recommended)

Run `Upload + Monitor (Safe)` from **Tasks: Run Task**.

Or use `Upload + Monitor (60s)` if you want it to stop automatically.

It will:
- upload firmware
- start serial monitor (`115200`)
- avoid common port-lock issues from old monitor sessions

## 📦 Versions (tested)

- PlatformIO Core: `6.x`
- Platform: `espressif32 @ 6.12.0`
- Framework: `arduinoespressif32 @ 3.20017.241212`
- Library: `GFX Library for Arduino @ 1.4.9`
- Library: `lvgl @ 9.2.2`
- Board config: `esp32-s3-devkitc-1` (`board_build.arduino.memory_type = qio_opi`)

## ⚙️ Config

Pin changes go in `include/AppConfig.h`.

LVGL buffer/color settings in `include/lv_conf.h`.

## 📊 Build Status

- **Flash**: 564 KB
- **RAM**: 112 KB
- **Build**: Zero warnings

## 🛠️ If It Doesn't Work

**Black screen**
- Check backlight (GPIO 1) is HIGH
- Check QSPI pins are correct

**Touch offset**
- Verify touch pins (GPIO 3, 4, 8)

**Build fails**
```bash
rm -rf .pio
pio pkg install
```

**See serial output:**
```bash
pio device monitor --port /dev/ttyACM0 --baud 115200
```

## 🧱 Code Structure

- `main.cpp` - entry point, minimal setup
- `DisplayManager.cpp` - display/LVGL/touch driver
- `SplashScreen.cpp` - boot splash + blink animation
- `AppUI.cpp` - UI scene
- `include/AppConfig.h` - GPIO pin config
interface

Enjoy!

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20me%20a%20coffee-karellangeraar-FFDD00?logo=buymeacoffee&logoColor=000000)](https://buymeacoffee.com/karellangeraar)
