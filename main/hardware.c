#include "hardware.h"

#include "driver/gpio.h"
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

void set_relais_status(uint32_t gpio_num, bool state){

    gpio_set_level(gpio_num, (uint32_t)state);
}