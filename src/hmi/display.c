#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include <sys/time.h>
#include <string.h>

#include "common/gpios.h"
#include "drivers/hd44780.h"
#include "drivers/pcf8574.h"

#define DEBUG true

static const char *TAG = "AC_Display";

static i2c_dev_t pcf8574;

static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data);
void lcd_test(void *pvParameters);

void Display_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreate(lcd_test, "lcd_test", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}

static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    return pcf8574_port_write(&pcf8574, data);
}

void lcd_test(void *pvParameters) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Fill LCD descriptor
    hd44780_t lcd = {
        .write_cb = write_lcd_data, // use callback to send data to LCD by I2C GPIO expander
        .font = HD44780_FONT_5X8,
        .lines = 2,
        .pins = {
            .rs = 0,
            .e  = 2,
            .d4 = 4,
            .d5 = 5,
            .d6 = 6,
            .d7 = 7,
            .bl = 3
        }
    };

    // Prepare PCF8574
    memset(&pcf8574, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(pcf8574_init_desc(&pcf8574, I2C_PCF8574_ADDRESS, 0, I2C_MASTER_GPIO_SDA, I2C_MASTER_GPIO_SCL));

    // Init screen, switch backlight on
    ESP_ERROR_CHECK(hd44780_init(&lcd));
    hd44780_switch_backlight(&lcd, true);

    char s[16];
    float pressure, temperature;

    while (1) {
        temperature = 0.2;
        pressure = 1000;

        // print data
        snprintf(s, sizeof(s), "t: %.01f C", temperature);
        hd44780_gotoxy(&lcd, 0, 0);
        hd44780_puts(&lcd, s);

        snprintf(s, sizeof(s), "P: %.02f kPa", pressure / 1000);
        hd44780_gotoxy(&lcd, 0, 1);
        hd44780_puts(&lcd, s);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
