#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

constexpr uint8_t TFT_BL = 1;
constexpr uint8_t TFT_QSPI_CS = 45;
constexpr uint8_t TFT_QSPI_SCK = 47;
constexpr uint8_t TFT_QSPI_D0 = 21;
constexpr uint8_t TFT_QSPI_D1 = 48;
constexpr uint8_t TFT_QSPI_D2 = 40;
constexpr uint8_t TFT_QSPI_D3 = 39;

constexpr uint8_t TOUCH_SCL = 8;
constexpr uint8_t TOUCH_SDA = 4;
constexpr uint8_t TOUCH_INT = 3;
constexpr uint8_t TOUCH_ADDR = 0x3B;

constexpr int32_t TFT_WIDTH = 320;
constexpr int32_t TFT_HEIGHT = 480;

#endif