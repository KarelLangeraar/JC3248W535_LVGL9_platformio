#include "AXS15231B_Display.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "AXS_Display";

#define BEGIN_WRITE     0x01
#define WRITE_C8_D8     0x02
#define WRITE_C8_D16    0x03
#define WRITE_C8_BYTES  0x04
#define END_WRITE       0x05
#define DELAY           0x06

// Some standard register addresses
#define AXS15231B_SWRESET 0x01
#define AXS15231B_SLPIN   0x10
#define AXS15231B_SLPOUT  0x11
#define AXS15231B_INVOFF  0x20
#define AXS15231B_INVON   0x21
#define AXS15231B_DISPOFF 0x28
#define AXS15231B_DISPON  0x29
#define AXS15231B_CASET   0x2A
#define AXS15231B_RASET   0x2B
#define AXS15231B_RAMWR   0x2C
#define AXS15231B_COLMOD  0x3A
#define AXS15231B_MADCTL  0x36
#define AXS15231B_PTLAR   0x30

static const uint8_t axs15231b_init_ops[] = {
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA0, 17,
    0xC0, 0x10, 0x00, 0x02, 0x00,
    0x00, 0x04, 0x3F, 0x20, 0x05,
    0x3F, 0x3F, 0x00, 0x00, 0x00,
    0x00, 0x00,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA2, 31,
    0x30, 0x3C, 0x24, 0x14, 0xD0,
    0x20, 0xFF, 0xE0, 0x40, 0x19,
    0x80, 0x80, 0x80, 0x20, 0xf9,
    0x10, 0x02, 0xff, 0xff, 0xF0,
    0x90, 0x01, 0x32, 0xA0, 0x91,
    0xE0, 0x20, 0x7F, 0xFF, 0x00,
    0x5A,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD0, 30,
    0xE0, 0x40, 0x51, 0x24, 0x08,
    0x05, 0x10, 0x01, 0x20, 0x15,
    0xC2, 0x42, 0x22, 0x22, 0xAA,
    0x03, 0x10, 0x12, 0x60, 0x14,
    0x1E, 0x51, 0x15, 0x00, 0x8A,
    0x20, 0x00, 0x03, 0x3A, 0x12,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA3, 22,
    0xA0, 0x06, 0xAA, 0x00, 0x08,
    0x02, 0x0A, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x00,
    0x55, 0x55,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC1, 30,
    0x31, 0x04, 0x02, 0x02, 0x71,
    0x05, 0x24, 0x55, 0x02, 0x00,
    0x41, 0x00, 0x53, 0xFF, 0xFF,
    0xFF, 0x4F, 0x52, 0x00, 0x4F,
    0x52, 0x00, 0x45, 0x3B, 0x0B,
    0x02, 0x0d, 0x00, 0xFF, 0x40,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC3, 11,
    0x00, 0x00, 0x00, 0x50, 0x03,
    0x00, 0x00, 0x00, 0x01, 0x80,
    0x01,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC4, 29,
    0x00, 0x24, 0x33, 0x80, 0x00,
    0xea, 0x64, 0x32, 0xC8, 0x64,
    0xC8, 0x32, 0x90, 0x90, 0x11,
    0x06, 0xDC, 0xFA, 0x00, 0x00,
    0x80, 0xFE, 0x10, 0x10, 0x00,
    0x0A, 0x0A, 0x44, 0x50,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC5, 23,
    0x18, 0x00, 0x00, 0x03, 0xFE,
    0x3A, 0x4A, 0x20, 0x30, 0x10,
    0x88, 0xDE, 0x0D, 0x08, 0x0F,
    0x0F, 0x01, 0x3A, 0x4A, 0x20,
    0x10, 0x10, 0x00,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC6, 20,
    0x05, 0x0A, 0x05, 0x0A, 0x00,
    0xE0, 0x2E, 0x0B, 0x12, 0x22,
    0x12, 0x22, 0x01, 0x03, 0x00,
    0x3F, 0x6A, 0x18, 0xC8, 0x22,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC7, 20,
    0x50, 0x32, 0x28, 0x00, 0xa2,
    0x80, 0x8f, 0x00, 0x80, 0xff,
    0x07, 0x11, 0x9c, 0x67, 0xff,
    0x24, 0x0c, 0x0d, 0x0e, 0x0f,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC9, 4,
    0x33, 0x44, 0x44, 0x01,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xCF, 27,
    0x2C, 0x1E, 0x88, 0x58, 0x13,
    0x18, 0x56, 0x18, 0x1E, 0x68,
    0x88, 0x00, 0x65, 0x09, 0x22,
    0xC4, 0x0C, 0x77, 0x22, 0x44,
    0xAA, 0x55, 0x08, 0x08, 0x12,
    0xA0, 0x08,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD5, 30,
    0x40, 0x8E, 0x8D, 0x01, 0x35,
    0x04, 0x92, 0x74, 0x04, 0x92,
    0x74, 0x04, 0x08, 0x6A, 0x04,
    0x46, 0x03, 0x03, 0x03, 0x03,
    0x82, 0x01, 0x03, 0x00, 0xE0,
    0x51, 0xA1, 0x00, 0x00, 0x00,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD6, 30,
    0x10, 0x32, 0x54, 0x76, 0x98,
    0xBA, 0xDC, 0xFE, 0x93, 0x00,
    0x01, 0x83, 0x07, 0x07, 0x00,
    0x07, 0x07, 0x00, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x00,
    0x84, 0x00, 0x20, 0x01, 0x00,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD7, 19,
    0x03, 0x01, 0x0b, 0x09, 0x0f,
    0x0d, 0x1E, 0x1F, 0x18, 0x1d,
    0x1f, 0x19, 0x40, 0x8E, 0x04,
    0x00, 0x20, 0xA0, 0x1F,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD8, 12,
    0x02, 0x00, 0x0a, 0x08, 0x0e,
    0x0c, 0x1E, 0x1F, 0x18, 0x1d,
    0x1f, 0x19,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD9, 12,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xDD, 12,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xDF, 8,
    0x44, 0x73, 0x4B, 0x69, 0x00,
    0x0A, 0x02, 0x90,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE0, 17,
    0x3B, 0x28, 0x10, 0x16, 0x0c,
    0x06, 0x11, 0x28, 0x5c, 0x21,
    0x0D, 0x35, 0x13, 0x2C, 0x33,
    0x28, 0x0D,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE1, 17,
    0x37, 0x28, 0x10, 0x16, 0x0b,
    0x06, 0x11, 0x28, 0x5C, 0x21,
    0x0D, 0x35, 0x14, 0x2C, 0x33,
    0x28, 0x0F,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE2, 17,
    0x3B, 0x07, 0x12, 0x18, 0x0E,
    0x0D, 0x17, 0x35, 0x44, 0x32,
    0x0C, 0x14, 0x14, 0x36, 0x3A,
    0x2F, 0x0D,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE3, 17,
    0x37, 0x07, 0x12, 0x18, 0x0E,
    0x0D, 0x17, 0x35, 0x44, 0x32,
    0x0C, 0x14, 0x14, 0x36, 0x32,
    0x2F, 0x0F,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE4, 17,
    0x3B, 0x07, 0x12, 0x18, 0x0E,
    0x0D, 0x17, 0x39, 0x44, 0x2E,
    0x0C, 0x14, 0x14, 0x36, 0x3A,
    0x2F, 0x0D,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE5, 17,
    0x37, 0x07, 0x12, 0x18, 0x0E,
    0x0D, 0x17, 0x39, 0x44, 0x2E,
    0x0C, 0x14, 0x14, 0x36, 0x3A,
    0x2F, 0x0F,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA4, 16,
    0x85, 0x85, 0x95, 0x82, 0xAF,
    0xAA, 0xAA, 0x80, 0x10, 0x30,
    0x40, 0x40, 0x20, 0xFF, 0x60,
    0x30,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA4, 4,
    0x85, 0x85, 0x95, 0x85,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xBB, 8,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    END_WRITE,

    BEGIN_WRITE,
    WRITE_C8_D8, 0x11, 0x00,
    END_WRITE,

    DELAY, 120,

    BEGIN_WRITE,
    WRITE_C8_D8, 0x29, 0x00,
    END_WRITE,

    DELAY, 100,

    BEGIN_WRITE,
    WRITE_C8_BYTES, 0x2C, 4,
    0x00, 0x00, 0x00, 0x00,
    END_WRITE
};

AXS15231B_Display::AXS15231B_Display(int width, int height) 
    : _width(width), _height(height), _spi(NULL) {
}

AXS15231B_Display::~AXS15231B_Display() {
    if (_spi) {
        spi_bus_remove_device(_spi);
    }
}

bool AXS15231B_Display::init(int cs, int sck, int d0, int d1, int d2, int d3, int rst, int speed_hz) {
    _rst_pin = rst;

    spi_bus_config_t buscfg = {};
    buscfg.data0_io_num = d0;
    buscfg.data1_io_num = d1;
    buscfg.data2_io_num = sck;
    buscfg.data3_io_num = -1;
    buscfg.data4_io_num = -1;
    buscfg.data5_io_num = -1;
    buscfg.data6_io_num = -1;
    buscfg.data7_io_num = -1;
    buscfg.max_transfer_sz = (_width * _height * 2) + 8;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;
    buscfg.intr_flags = 0;
    
    // Assign proper QSPI pins (data2/3) and sclk
    buscfg.mosi_io_num = d0;
    buscfg.miso_io_num = d1;
    buscfg.sclk_io_num = sck;
    buscfg.quadwp_io_num = d2;
    buscfg.quadhd_io_num = d3;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus.");
        return false;
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 8;
    devcfg.address_bits = 24;
    devcfg.dummy_bits = 0;
    devcfg.mode = 0;
    devcfg.duty_cycle_pos = 0;
    devcfg.cs_ena_pretrans = 0;
    devcfg.cs_ena_posttrans = 0;
    devcfg.clock_speed_hz = speed_hz;
    devcfg.input_delay_ns = 0;
    devcfg.spics_io_num = cs;
    devcfg.flags = SPI_DEVICE_HALFDUPLEX;
    devcfg.queue_size = 7;
    devcfg.pre_cb = NULL;
    devcfg.post_cb = NULL;

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device.");
        return false;
    }

    hwReset();

    // Software reset
    writeCommand(AXS15231B_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(120));

    sendInitSequence();

    // Additional configuration translated from Arduino AXS15231B initialization
    uint8_t colmod_data = 0x55;
    writeCommandBytes(AXS15231B_COLMOD, &colmod_data, 1);
    
    writeC8D16D16(AXS15231B_PTLAR, 0x0000, 0x01DF); // 0, 479
    writeCommand(0x13);

    // Invert OFF
    writeCommand(AXS15231B_INVOFF);

    return true;
}

void AXS15231B_Display::hwReset() {
    if (_rst_pin >= 0) {
        gpio_set_direction((gpio_num_t)_rst_pin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)_rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level((gpio_num_t)_rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(120));
        gpio_set_level((gpio_num_t)_rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }
}

// Emulates Arduino_ESP32QSPI command layout base mapping: 0x02 instruction, 24-bit address for registers
void AXS15231B_Display::writeCommand(uint8_t cmd) {
    spi_transaction_ext_t t = {};
    t.base.flags = SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR;
    t.base.cmd = 0x02;
    t.base.addr = ((uint32_t)cmd) << 8;
    t.base.length = 0;
    spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
}

void AXS15231B_Display::writeCommandBytes(uint8_t cmd, const uint8_t *data, size_t len) {
    spi_transaction_ext_t t = {};
    t.base.flags = SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR;
    t.base.cmd = 0x02;
    t.base.addr = ((uint32_t)cmd) << 8;
    t.base.length = len * 8;
    
    // For small configuration payloads, copy data into the internal SPI transaction buffer directly
    // This avoids DMA memory access violations if 'data' is given from a PSRAM task stack
    if (len <= 4 && data != nullptr) {
        t.base.flags |= SPI_TRANS_USE_TXDATA;
        memcpy(t.base.tx_data, data, len);
    } else {
        t.base.tx_buffer = data;
    }
    
    if (_spi == NULL) {
        ESP_LOGE("AXS", "_spi is NULL in writeCommandBytes! This is %p", this);
        return;
    }
    
    spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
}

void AXS15231B_Display::writeC8D16D16(uint8_t cmd, uint16_t d1, uint16_t d2) {
    uint8_t buf[4];
    buf[0] = d1 >> 8;
    buf[1] = d1 & 0xFF;
    buf[2] = d2 >> 8;
    buf[3] = d2 & 0xFF;
    writeCommandBytes(cmd, buf, 4);
}

void AXS15231B_Display::sendInitSequence() {
    const uint8_t *p = axs15231b_init_ops;
    size_t len = sizeof(axs15231b_init_ops);
    size_t i = 0;
    
    while (i < len) {
        uint8_t typ = p[i++];
        switch(typ) {
            case BEGIN_WRITE:
                break;
            case END_WRITE:
                break;
            case DELAY: {
                uint8_t ms = p[i++];
                vTaskDelay(pdMS_TO_TICKS(ms));
                break;
            }
            case WRITE_C8_BYTES: {
                uint8_t cmd = p[i++];
                uint8_t count = p[i++];
                writeCommandBytes(cmd, &p[i], count);
                i += count;
                break;
            }
            case WRITE_C8_D8: {
                uint8_t cmd = p[i++];
                uint8_t val = p[i++];
                writeCommandBytes(cmd, &val, 1);
                break;
            }
        }
    }
}

void AXS15231B_Display::drawBitmapQSPI(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
    // ESP32-S3 max SPI transaction length is 32768 bytes (262144 bits).
    const uint16_t CHUNK_ROWS = 50;

    // Set the window once
    writeC8D16D16(AXS15231B_CASET, x, x + w - 1);
    writeC8D16D16(AXS15231B_RASET, y, y + h - 1);

    for (uint16_t y_offset = 0; y_offset < h; y_offset += CHUNK_ROWS) {
        uint16_t current_h = (h - y_offset > CHUNK_ROWS) ? CHUNK_ROWS : (h - y_offset);
        
        // 4. Send pixel data via QSPI mode 0x32
        spi_transaction_ext_t t = {};
        t.base.flags = SPI_TRANS_MODE_QIO;  // QSPI mode for data
        t.base.cmd = 0x32; 
        
        // First block gets RAMWR (0x2C), subsequent blocks get RAMWRC (0x3C) to continue writing
        t.base.addr = (y_offset == 0) ? 0x002C00 : 0x003C00;
        
        t.base.length = w * current_h * 16;
        t.base.tx_buffer = bitmap + (y_offset * w);

        if (_spi == NULL) {
            ESP_LOGE("AXS", "_spi is NULL in loop!");
        }

        esp_err_t ret = spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
        if (ret != ESP_OK) {
            ESP_LOGE("AXS", "SPI transmit failed: %d", ret);
        }
    }
}
