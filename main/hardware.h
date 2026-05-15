#pragma once

#include <inttypes.h>
#include <stdbool.h>

// Pin-Defines (öffentlich)

#define PIN_RELAIS_1 4
#define PIN_RELAIS_2 0
#define PIN_RELAIS_3 2
#define PIN_RELAIS_4 15
#define PIN_RELAIS_5 13
#define PIN_RELAIS_6 12
#define PIN_RELAIS_7 14
#define PIN_RELAIS_8 27

// Interrupt-Übergabewert
extern volatile bool isr_handle_change;

void configure_gpios(void);
void set_relais_status(uint32_t gpio_num, bool state);