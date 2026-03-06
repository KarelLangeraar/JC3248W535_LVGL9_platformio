#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "hal/spi_types.h"

// Basic driver for the AXS15231B display connected via QSPI
class AXS15231B_Display {
public:
    AXS15231B_Display(int width = 320, int height = 480);
    ~AXS15231B_Display();

    bool init(int cs, int sck, int d0, int d1, int d2, int d3, int rst = -1, int speed_hz = 32000000);
    
    // Draw directly a window
    void drawBitmapQSPI(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap);

private:
    int _width;
    int _height;
    int _rst_pin;
    spi_device_handle_t _spi;

    void writeCommand(uint8_t cmd);
    void writeCommandBytes(uint8_t cmd, const uint8_t *data, size_t len);
    void writeC8D16D16(uint8_t cmd, uint16_t d1, uint16_t d2);
    void hwReset();
    void sendInitSequence();
};
