#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hardware.h"

void app_main(void)
{

    printf("Testing Interrupt...\n\n");
    configure_gpios();

    bool relais_state = false;

    while(1){

        bool interrupt_last_state = isr_handle_change;
        vTaskDelay(pdMS_TO_TICKS(10));

        if(isr_handle_change != interrupt_last_state){

            isr_handle_change = false;
            relais_state = !relais_state;
            set_relais_status(PIN_RELAIS_1, relais_state);
            printf("Interrupt triggered!, Relais-State: %d\n", relais_state);
        }
    }
}