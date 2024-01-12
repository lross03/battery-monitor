/*
LCD Pins            ESP32 Pins
VSS                 GND
VDD                 5V
V0                  GND
RS                  33
RW                  5
E                   19
D0                  12
D1                  13
D2                  14
D3                  18
D4                  25
D5                  26
D6                  27
D7                  32
LED+                5V
LED-                GND

*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <math.h>
#include <string.h>

#define D0 12
#define D1 13
#define D2 14
#define D3 18
#define D4 25
#define D5 26
#define D6 27
#define D7 32
#define RW 5
#define RS 33
#define E 19

//int lcd_pins[11] = {12,13,14,18,25,26,27,32,33,5,19};
int lcd_pins[11] = {D0, D1, D2, D3, D4, D5, D6, D7, RS, RW, E};
//char init_msg[] = "LCD Initialized";


void lcd_decode(unsigned char message) {
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(lcd_pins[i], (message & (1 << i)) ? 1 : 0);
    }
}

void lcd_cmd(unsigned char cmd) {
    lcd_decode(cmd);
    gpio_set_level(lcd_pins[8],0);
    gpio_set_level(lcd_pins[9],0);
    gpio_set_level(lcd_pins[10],1);
    vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(lcd_pins[10], 0);
    vTaskDelay(10/portTICK_PERIOD_MS);
}

void lcd_data(unsigned char data) {
    lcd_decode(data);
    gpio_set_level(lcd_pins[8],1);
    gpio_set_level(lcd_pins[9],0);
    gpio_set_level(lcd_pins[10],1);
    vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(lcd_pins[10], 0);
    vTaskDelay(10/portTICK_PERIOD_MS);
}

void lcd_message(char *p) {
    lcd_cmd(0x01);
    vTaskDelay(50/portTICK_PERIOD_MS);
    lcd_cmd(0x80);
    int i = 0;
    while (*p != '\0') {
        //printf("%s\n", p);
        while (i >= 16 && i < 40) {
            char *t = "o";
            lcd_data(*t);
            i++;
        }
        lcd_data(*p);
        p = p + 1;
        i++;
    }
}

void lcd_percentage(char *p) {
    vTaskDelay(50/portTICK_PERIOD_MS);
    lcd_cmd(0x80);
    int i = 0;
    while (*p != '\0') {
        while (i >= 16 && i < 40) {
            char *t = "o";
            lcd_data(*t);
            i++;
        }
        //printf("%s\n", p);
        lcd_data(*p);
        p = p + 1;
        i++;        
    }
}

void lcd_init() {
    for (int i = 0; i < 11; i++) {
        gpio_reset_pin(lcd_pins[i]);
        gpio_set_direction(lcd_pins[i], GPIO_MODE_OUTPUT);
    }
    printf("initializing\n");
    //configure to 8 bit mode
    lcd_cmd(0x38);
    vTaskDelay(100/portTICK_PERIOD_MS);

    //clear display screen
    lcd_cmd(0x01);
    vTaskDelay(100/portTICK_PERIOD_MS);
    //display cursor on and display on
    lcd_cmd(0x0E);
    vTaskDelay(100/portTICK_PERIOD_MS);
    //set cursor to first line first position
    lcd_cmd(0x80);
    vTaskDelay(100/portTICK_PERIOD_MS);
    //print message to the screen
    //lcd_string(init_msg);
    //vTaskDelay(1000/portTICK_PERIOD_MS);
}



