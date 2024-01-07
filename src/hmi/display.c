/**
 * @file display.c
 * @brief Module for managing the LCD display and handling display-related events.
 *
 * This module includes functions to initialize and manage the LCD display, as well as handle events that trigger
 * changes in the display content.
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/events.h"
#include "common/gpios.h"
#include "common/composter_parameters.h"
#include "drivers/hd44780.h"
#include "drivers/pcf8574.h"

#define DEBUG false
#define DISPLAY_CONNECTED true
#define DISPLAY_CHAR_ROWS 2
#define DISPLAY_CHAR_COLUMNS 16

static const char *TAG = "AC_Display";

// Strings for different display messages
static char main_msg[] = "AutoCompost";
static char welcome_msg[] = "Welcome AutoComposter!";
static char wifi_connected_msg[] = "Wi-Fi connected";
static char wifi_disconnected_msg[] = "Wi-Fi disconnected";
static char mixer_on_msg[] = "Mixer ON";
static char mixer_off_msg[] = "Mixer OFF";
static char crusher_on_msg[] = "Crusher ON";
static char crusher_off_msg[] = "Crusher OFF";
static char fan_on_msg[] = "Fan ON";
static char fan_off_msg[] = "Fan OFF";
static char request_to_close_lid_to_crush_msg[] = "Please, close the lid NOW!";
static char request_to_close_lid_msg[] = "It stinks, close the lid NOW!";
static char request_to_empty_composter_msg[] = "Please, empty composter";

extern ComposterParameters composterParameters;
static i2c_dev_t pcf8574;
static bool event_received = false;
static char new_message[DISPLAY_CHAR_ROWS][DISPLAY_CHAR_COLUMNS];

static i2c_dev_t pcf8574;

static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data);
static void split_message(const char *input, char *line1, char *line2);
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void lcd_task(void *pvParameters);

/**
 * @brief Function to start the display module.
 */
void Display_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

#ifdef DISPLAY_CONNECTED
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreate(lcd_task, "lcd_task", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);

    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, MIXER_EVENT_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, MIXER_EVENT_OFF, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, CRUSHER_EVENT_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, CRUSHER_EVENT_OFF, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(FAN_EVENT, FAN_EVENT_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(FAN_EVENT, FAN_EVENT_OFF, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT_INTERNAL, WIFI_EVENT_CONNECTION_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT_INTERNAL, WIFI_EVENT_CONNECTION_OFF, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(LOCK_EVENT, LOCK_EVENT_REQUEST_TO_CLOSE_LID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(LOCK_EVENT, LOCK_EVENT_REQUEST_TO_EMPTY_COMPOSTER, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(LID_EVENT, LID_EVENT_REQUEST_TO_CLOSE_LID, &event_handler, NULL));
#endif
}

/**
 * @brief Event handler function for various events that trigger changes in the display content.
 *
 * @param arg         Unused parameter.
 * @param event_base  Event base of the received event.
 * @param event_id    Event ID of the received event.
 * @param event_data  Data associated with the received event.
 */
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    // Handle different events and set corresponding messages for display
    if (strcmp(event_base, WIFI_EVENT_INTERNAL) == 0) {
        if (event_id == WIFI_EVENT_CONNECTION_ON) {
            split_message(wifi_connected_msg, new_message[0], new_message[1]);
        } else if (event_id == WIFI_EVENT_CONNECTION_OFF) {
            split_message(wifi_disconnected_msg, new_message[0], new_message[1]);
        }
    } else if (strcmp(event_base, MIXER_EVENT) == 0) {
        if (event_id == MIXER_EVENT_ON) {
            split_message(mixer_on_msg, new_message[0], new_message[1]);
        } else if (event_id == MIXER_EVENT_OFF) {
            split_message(mixer_off_msg, new_message[0], new_message[1]);
        }
    } else if (strcmp(event_base, CRUSHER_EVENT) == 0) {
        if (event_id == CRUSHER_EVENT_ON) {
            split_message(crusher_on_msg, new_message[0], new_message[1]);
        } else if (event_id == CRUSHER_EVENT_OFF) {
            split_message(crusher_off_msg, new_message[0], new_message[1]);
        }
    } else if (strcmp(event_base, FAN_EVENT) == 0) {
        if (event_id == FAN_EVENT_ON) {
            split_message(fan_on_msg, new_message[0], new_message[1]);
        } else if (event_id == FAN_EVENT_OFF) {
            split_message(fan_off_msg, new_message[0], new_message[1]);
        }
    } else if (strcmp(event_base, LOCK_EVENT) == 0) {
        if (event_id == LOCK_EVENT_REQUEST_TO_CLOSE_LID) {
            split_message(request_to_close_lid_to_crush_msg, new_message[0], new_message[1]);
        } else if (event_id == LOCK_EVENT_REQUEST_TO_EMPTY_COMPOSTER) {
            split_message(request_to_empty_composter_msg, new_message[0], new_message[1]);
        }
    } else if (strcmp(event_base, LID_EVENT) == 0) {
        if (event_id == LID_EVENT_REQUEST_TO_CLOSE_LID) {
            split_message(request_to_close_lid_msg, new_message[0], new_message[1]);
        }
    }

    event_received = true;
}

/**
 * @brief Function to split a message into two lines to fit the display.
 *
 * @param input  Input message to split.
 * @param line1  Output buffer for the first line.
 * @param line2  Output buffer for the second line.
 */
static void split_message(const char *input, char *line1, char *line2) {
    int length = strlen(input);
    if (length <= DISPLAY_CHAR_COLUMNS) {
        // The message fits in one line
        strcpy(line1, input);
        line2[0] = '\0';  // The second line is empty
    } else {
        int split_pos = DISPLAY_CHAR_COLUMNS;
        if (input[split_pos] != ' ') {
            // Find the position of the last space before DISPLAY_CHAR_COLUMNS
            while (split_pos > 0 && input[split_pos] != ' ') {
                split_pos--;
            }
            if (split_pos == 0) {
                // No space found before DISPLAY_CHAR_COLUMNS, so cut at DISPLAY_CHAR_COLUMNS
                split_pos = DISPLAY_CHAR_COLUMNS;
            }
        }

        strncpy(line1, input, split_pos);
        line1[split_pos] = '\0';

        // Move to the first unprocessed character
        while (input[split_pos] == ' ') {
            split_pos++;
        }

        strcpy(line2, input + split_pos);

        // Ensure the second line does not exceed DISPLAY_CHAR_COLUMNS
        if (strlen(line2) > DISPLAY_CHAR_COLUMNS) {
            line2[DISPLAY_CHAR_COLUMNS] = '\0';
        }
    }
}

/**
 * @brief Callback function to write data to the LCD.
 *
 * @param lcd   Pointer to the LCD descriptor.
 * @param data  Data to write to the LCD.
 * @return ESP_OK on success, otherwise an error code.
 */
static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data) {
    return pcf8574_port_write(&pcf8574, data);
}

/**
 * @brief Task to manage the LCD display.
 *
 * @param pvParameters  Unused parameter.
 */
void lcd_task(void *pvParameters) {
    char parameters_msg[DISPLAY_CHAR_COLUMNS];
    float temperature;
    float humidity;

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
    ESP_ERROR_CHECK(pcf8574_init_desc(&pcf8574, I2C_PCF8574_ADDRESS, I2C_NUM_0, I2C_MASTER_GPIO_SDA, I2C_MASTER_GPIO_SCL));

    // Init screen, switch backlight on
    ESP_ERROR_CHECK(hd44780_init(&lcd));
    hd44780_switch_backlight(&lcd, true);
    hd44780_clear(&lcd);

    split_message(welcome_msg, new_message[0], new_message[1]);
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_puts(&lcd, new_message[0]);
    hd44780_gotoxy(&lcd, 0, 1);
    hd44780_puts(&lcd, new_message[1]);

    TickType_t event_start_time = 1; // Start time of displaying the event message

    while (true) {
        // Check if an event has arrived and set the event message if necessary
        if (event_received) {
            event_received = false; // Reset event state
            hd44780_clear(&lcd);
            if (strlen(new_message[0])) {
                hd44780_gotoxy(&lcd, 0, 0);
                hd44780_puts(&lcd, new_message[0]);
                if (DEBUG) ESP_LOGI(TAG, "on %s: new msg[0]: %s", __func__, new_message[0]);
            }
            if (strlen(new_message[1])) {
                hd44780_gotoxy(&lcd, 0, 1);
                hd44780_puts(&lcd, new_message[1]);
                if (DEBUG) ESP_LOGI(TAG, "on %s: new msg[1]: %s", __func__, new_message[1]);
            }
            event_start_time = xTaskGetTickCount(); // Record the start time of displaying the event message
        }

        if (xTaskGetTickCount() >= event_start_time + pdMS_TO_TICKS(5000)) {
            if (event_start_time) {
                event_start_time = 0; // Reset the start time
                hd44780_clear(&lcd);
            }
            hd44780_gotoxy(&lcd, 0, 0);
            hd44780_puts(&lcd, main_msg);

            // Get temperature and humidity values
            temperature = ComposterParameters_GetTemperature(&composterParameters);
            humidity = ComposterParameters_GetHumidity(&composterParameters);

            // Format the string with both values
            // ° is \xdf
            snprintf(parameters_msg, sizeof(parameters_msg), "T: %.0f H: %.0f %%", temperature, humidity);

            // Display the string on the screen
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, parameters_msg);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Update the display every 1000 ms
    }
}
