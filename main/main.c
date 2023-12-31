#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define BUF_SIZE (1024)
#define UART_NUM UART_NUM_0 
#define LED_PIN 2    

void app_main() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    uint8_t* data = (uint8_t*)malloc(BUF_SIZE);

    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = 0; 

            for (int i = 0; i < len; i++) {
                if (data[i] == '\n') {
                    data[i] = 0; 
                    break;
                }
            }
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            gpio_set_level(LED_PIN, 0); 
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            int batteryPercentage = atoi((const char*)data);

            if (batteryPercentage >= 0 && batteryPercentage <= 100) {
                // Blink the LED if receiving valid battery percentage
                
            }
        }

        vTaskDelay(500 / portTICK_PERIOD_MS); 
        free(data);
        data = (uint8_t*)malloc(BUF_SIZE);
    }

    free(data);
}