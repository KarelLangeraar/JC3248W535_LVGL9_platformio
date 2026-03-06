#pragma once

#include <stdint.h>
#include <stdbool.h>

class AXS15231B_Touch_IDF {
public:
    AXS15231B_Touch_IDF(uint8_t scl, uint8_t sda, uint8_t int_pin, uint8_t addr, uint8_t rotation = 0);
    ~AXS15231B_Touch_IDF();

    bool begin();
    bool touched();
    void setRotation(uint8_t rot);
    void readData(uint16_t *x, uint16_t *y);

private:
    uint8_t _scl;
    uint8_t _sda;
    uint8_t _int_pin;
    uint8_t _addr;
    uint8_t _rotation;

    volatile bool _touch_int = false;
    uint16_t _point_X = 0;
    uint16_t _point_Y = 0;

    bool update();
    static void isr_handler(void* arg);
};
