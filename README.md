# Working JC3248W535 LVGL display (ESP-IDF Native)

I got the cheap and widely available **JC3248W535** hardware, but had a hard time getting it to behave well with LVGL using the typical Arduino framework. After battling intense memory constraints and `LoadProhibited` DMA crashes, this project has been fully ported to **Native ESP-IDF**.

This code is meant as a starting point for development so you don't have to figure it out from scratch. 

My primary purpose is to build a smart lightswitch for WLED, hence the project name.

## ✨ What It Does

- **Native ESP-IDF Architecture:** Bypasses Arduino framework limitations, offering direct control over precise memory alignment to prevent rendering crashes.
- **Optimized Rendering Engine:** Custom layout pushes contiguous partial pixel buffers using an intermediate PSRAM full-framebuffer to prevent QSPI screen scrambling, keeping drawing tear-free and stable at maximum 40MHz SPI.
- **EEZ Studio Integration:** UI is visually designed in EEZ Studio and fully integrated via exported C-code.
- **XIP (Execute-In-Place) Images:** Re-mapped to a custom 16MB partition layout (10MB App space) allowing massive full-screen images (PNG/RGB565) to be rendered directly from flash at hardware speeds without SPIFFS buffering overhead.
- **Live Hardware Control:** On-screen sliders bound instantly to real-world hardware (e.g., hardware PWM backlight dimming).
- **LVGL Performance Telemetry:** Optimized settings with built-in LVGL 9 support.
- **Hardware Abstraction:** The driver components for the screen and touch are segregated natively within the custom `lib/AXS15231B/` PlatformIO library feature.
- **20-40 FPS** performance depending on graphical complexity.

## ⚠️ On your own...
This code comes as it is. It is a POC and I am certainly no hardware specialist. I am not planning on active monitoring or maintaining the code, but I might...

The upside? You are free to use it in any way you like ;)

## 🧰 Hardware

- **Board**: JC3248W535 (ESP32-S3)
- **Memory**: 512KB SRAM, 8MB PSRAM, 16MB Flash
- **Display**: 320x480 QSPI TFT (AXS15231B_JC3248) - _Note: Flex cable signal integrity limits SPI clock to ~40MHz._
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

1. **Install PlatformIO**: Using the VS Code extension or running `pip install platformio`
2. **Clone and build:**
   ```bash
   git clone <repo>
   cd LightSwitch
   pio run -j 8
   ```

3. **Upload:**
   ```bash
   pio run --target upload
   ```

### ✅ VS Code tasks (recommended)
Run `Upload + Monitor` from the built-in VS Code tasks. It will:
- compile via ESP-IDF
- flash the custom 16MB layout
- avoid common port-lock issues

## ⚙️ Memory & Partitions (Important)
Unlike standard Arduino scripts restricted to small application binaries, this project utilizes a custom `partitions.csv` scaling the application partition to **10MB**, with a complementary **5MB SPIFFS** partition.

We use **Direct Flash (XIP)** over SPIFFS for UI assets. SPIFFS is slow and forces the CPU to copy data into RAM. By placing UI image assets in the generous 10MB App partition, the ESP32 utilizes Execute-in-Place to stream graphics instantly at hardware speed, bypassing the CPU and saving RAM.

## 📦 Frameworks & Tools
- **Platform**: `espressif32` 
- **Framework**: `espidf` (Native!)
- **Libraries**: `lvgl @ ^9.2.2`
- **UI Editor**: [EEZ Studio](https://www.envox.eu/studio/studio-introduction/) (Project files located in `LightSwitchEez/`)

### 🎨 EEZ Studio Workflow
This project uses EEZ Studio as the visual UI builder. The `.eez-project` file lives in the `LightSwitchEez/` directory.

1. **Open** `LightSwitchEez.eez-project` in EEZ Studio.
2. **Design** your screens, add images, and define actions/variables.
3. **Build/Generate** the code entirely within EEZ Studio (it outputs directly to `LightSwitchEez/src/ui/`).
4. **Compile** normally in PlatformIO (`pio run`). The generated C-code is automatically compiled into the ESP-IDF build via `LightSwitchEez/CMakeLists.txt`.
5. **Bridge Logic**: Implement any UI callbacks in `/src/ui_actions.cpp` to tie EEZ Flow variables to real device hardware.

## 🧱 Code Structure
- `/src/main.cpp` - Entry point and setup.
- `/src/DisplayManager.cpp` - Core framework, LCD QSPI routing, hardware backlight PWM, LVGL driver registration, and partial bound flush optimisations.
- `/src/ui_actions.cpp` - The bridge intercepting EEZ UI variables/Flow (e.g. `set_var_test_value()`) to trigger real hardware features.
- `/lib/AXS15231B/` - Clean hardware wrapper for the AXS15231B QSPI driver and I2C touch controller.
- `/partitions.csv` - Defines the 16MB memory mapping.
- `/LightSwitchEez/` - Holds your EEZ Studio `.eez-project` file and generated UI code (`ui.c`, `images.c`, etc.). We also keep the `/resources` folder here for original image assets.
- `/include/lv_conf.h` - LVGL configuration tailored for ESP-IDF.

## 🛠️ Troubleshooting
**Build fails or strange LVGL issues:**
1. Clean the project completely (`pio run -t clean` or delete `.pio/` and `sdkconfig` to force a complete CMake rebuild).
2. Ensure EEZ Studio generated code cleanly without old artifacts.

**Display artifacting / Duplicate Screens:**
- Do not increase the SPI speed beyond `40000000` (40MHz) in `DisplayManager.cpp` -> `gfx->init()`. The default rigid flex cables provided on generic board clones do not have the signal integrity for 80MHz QSPI.

**Black screen:**
- Check backlight (GPIO 1) wiring / PWM mapping.

Enjoy!

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20me%20a%20coffee-karellangeraar-FFDD00?logo=buymeacoffee&logoColor=000000)](https://buymeacoffee.com/karellangeraar)
