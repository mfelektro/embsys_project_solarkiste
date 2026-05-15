#include "hardware.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_attr.h"

// Pin-Defines

#define PIN_TEMP_MPPT 34
#define PIN_TEMP_PSU 35

#define PIN_PWM_FAN 19

#define PIN_BTN_INTERRUPT 17

#define PIN_I2C_SCL 22
#define PIN_I2C_SDA 21


volatile bool isr_handle_change = false;


// Interrupt-Function
static void IRAM_ATTR isr_fun(void *args) {

    volatile bool *state = (volatile bool *)args;
    *state = !(*state);
}

void configure_gpios(void){

    gpio_config_t relais_config = {
        .pin_bit_mask = (1ULL << PIN_RELAIS_1) |
                        (1ULL << PIN_RELAIS_2) |
                        (1ULL << PIN_RELAIS_3) |
                        (1ULL << PIN_RELAIS_4) |
                        (1ULL << PIN_RELAIS_5) |
                        (1ULL << PIN_RELAIS_6) |
                        (1ULL << PIN_RELAIS_7) |
                        (1ULL << PIN_RELAIS_8),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    gpio_config_t bnt_interrupt_config = {
        .pin_bit_mask = (1ULL << PIN_BTN_INTERRUPT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config(&relais_config);
    gpio_config(&bnt_interrupt_config);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_BTN_INTERRUPT, isr_fun, (void*)&isr_handle_change);
}

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t ads1115_handle;

void configure_i2c(void){

    i2c_master_bus_config_t i2c_mst_config = {

        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = PIN_I2C_SCL,
        .sda_io_num = PIN_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t ads1115_cfg = {

        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0b1001000,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ads1115_cfg, &ads1115_handle));
}

void set_relais_status(uint32_t gpio_num, bool state){

    gpio_set_level(gpio_num, (uint32_t)state);
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

float read_ads1115_voltage(void) {
    
    // 1. Config Register beschreiben
    // Wir senden 3 Bytes: [Pointer-Register] [Config MSB] [Config LSB]
    // Config: OS(1) = Messung starten | MUX(100) = AIN0 | PGA(000) = +/-6.144V | MODE(1) = Single-Shot
    // DR(100) = 128 Samples/s | COMP(00011) = Disable Comparator
    // Binär: 1100 0001 1000 0011 -> Hex: 0xC183
    
    uint8_t write_buf[3];
    write_buf[0] = 0x01; // Pointer Register auf 'Config' setzen
    write_buf[1] = 0xC1; // Config Byte 1 (MSB)
    write_buf[2] = 0x83; // Config Byte 2 (LSB)

    // Senden mit 1000ms Timeout
    ESP_ERROR_CHECK(i2c_master_transmit(ads1115_handle, write_buf, sizeof(write_buf), 1000));

    // 2. Warten, bis die Konvertierung fertig ist.
    // Bei 128 SPS (Samples per Second) dauert eine Messung knapp 8ms. 
    // Wir warten zur Sicherheit 10ms.
    vTaskDelay(pdMS_TO_TICKS(10));

    // 3. Pointer Register auf 'Conversion' (Ergebnis) setzen
    uint8_t pointer_reg = 0x00;
    ESP_ERROR_CHECK(i2c_master_transmit(ads1115_handle, &pointer_reg, 1, 1000));

    // 4. Zwei Bytes vom Chip lesen
    uint8_t read_buf[2];
    ESP_ERROR_CHECK(i2c_master_receive(ads1115_handle, read_buf, sizeof(read_buf), 1000));

    // 5. Die zwei 8-Bit Werte zu einem 16-Bit Integer mit Vorzeichen zusammensetzen
    int16_t raw_adc = (read_buf[0] << 8) | read_buf[1];

    // 6. Rohwert in eine tatsächliche Spannung umrechnen.
    // Bei einem Bereich von +/- 6.144V und 16-Bit (32768 Schritte pro Polarität)
    // entspricht 1 Bit exakt 0.1875 mV (also 0.0001875 V).
    float voltage = raw_adc * (6.144f / 32768.0f);

    return voltage;
}