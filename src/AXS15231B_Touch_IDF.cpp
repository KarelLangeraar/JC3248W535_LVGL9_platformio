#include "AXS15231B_Touch_IDF.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "AXS_Touch";

#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      400000
#define I2C_MASTER_TIMEOUT_MS   10

#define AXS_GET_POINT_X(buf)       (((uint16_t)(buf[2] & 0x0F) << 8) + (uint16_t)buf[3])
#define AXS_GET_POINT_Y(buf)       (((uint16_t)(buf[4] & 0x0F) << 8) + (uint16_t)buf[5])

AXS15231B_Touch_IDF::AXS15231B_Touch_IDF(uint8_t scl, uint8_t sda, uint8_t int_pin, uint8_t addr, uint8_t rotation)
    : _scl(scl), _sda(sda), _int_pin(int_pin), _addr(addr), _rotation(rotation) {
}

AXS15231B_Touch_IDF::~AXS15231B_Touch_IDF() {
    i2c_driver_delete(I2C_MASTER_NUM);
    gpio_isr_handler_remove((gpio_num_t)_int_pin);
}

static void IRAM_ATTR touch_isr_handler(void* arg) {
    bool* flag = (bool*)arg;
    *flag = true;
}

bool AXS15231B_Touch_IDF::begin() {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = _sda;
    conf.scl_io_num = _scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;

    i2c_param_config(I2C_MASTER_NUM, &conf);
    esp_err_t ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Setup INT pin
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << _int_pin);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)_int_pin, touch_isr_handler, (void*)&_touch_int);

    ESP_LOGI(TAG, "Touch initialized");
    return true;
}

void AXS15231B_Touch_IDF::setRotation(uint8_t rot) {
    _rotation = rot;
}

bool AXS15231B_Touch_IDF::update() {
    if (!_touch_int) {
        return false;
    }
    _touch_int = false;

    uint8_t tmp_buf[8] = {0};
    uint8_t read_touchpad_cmd[8] = {0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00, 0x00, 0x08};
    
    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM, _addr,
        read_touchpad_cmd, sizeof(read_touchpad_cmd),
        tmp_buf, sizeof(tmp_buf),
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK) {
        return false;
    }

    uint16_t raw_X = AXS_GET_POINT_X(tmp_buf);
    uint16_t raw_Y = AXS_GET_POINT_Y(tmp_buf);

    if (raw_X > 319) raw_X = 319;
    if (raw_Y > 479) raw_Y = 479;

    switch (_rotation) {
        case 0:
            _point_X = raw_X;
            _point_Y = raw_Y;
            break;
        case 1:
            _point_X = raw_Y;
            _point_Y = 319 - raw_X;
            break;
        case 2:
            _point_X = 319 - raw_X;
            _point_Y = 479 - raw_Y;
            break;
        case 3:
            _point_X = 479 - raw_Y;
            _point_Y = raw_X;
            break;
    }
    
    return true;
}

bool AXS15231B_Touch_IDF::touched() {
    return update();
}

void AXS15231B_Touch_IDF::readData(uint16_t *x, uint16_t *y) {
    *x = _point_X;
    *y = _point_Y;
}
