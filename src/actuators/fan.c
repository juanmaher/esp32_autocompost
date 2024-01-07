/**
 * @file fan.c
 * @brief Implementation of a controller for the fan in a composting system.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Inclusion of FreeRTOS and ESP-IDF libraries
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

// Inclusion of custom header files
#include "common/composter_parameters.h"
#include "common/events.h"
#include "common/gpios.h"
#include "actuators/fan.h"

#define DEBUG false

#define START_FAN_TIMER_MS 2 * 60 * 1000

// Definition of events related to the fan
ESP_EVENT_DEFINE_BASE(FAN_EVENT);

// Tag to identify log messages
static const char *TAG = "AC_Fan";

// Variable to store the current state of the fan (on/off)
static bool fanOn;

// Timer handler for the fan
static TimerHandle_t fanTimer = NULL;

// External reference to composting parameters
extern ComposterParameters composterParameters;

// Declaration of internal functions
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();
static void timer_callback_function(TimerHandle_t xTimer);

/**
 * @brief Initializes the fan controller.
 *
 * Configures the fan GPIO and registers necessary event handlers.
 */
void Fan_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    fanOn = false;

    // Configuration of the fan GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FAN_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // Creation of the fan timer
    fanTimer = xTimerCreate("FanTimer", pdMS_TO_TICKS(START_FAN_TIMER_MS), pdTRUE, NULL, timer_callback_function);

    // Registration of event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_FAN_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(BUTTON_EVENT, BUTTON_EVENT_FAN_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PARAMETERS_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
}

/**
 * @brief Event handler for the fan.
 *
 * Handles events related to manual fan control, button presses, and parameter changes.
 * Turns on or off the fan as needed.
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, COMMUNICATOR_EVENT) == 0) {
        if (event_id == COMMUNICATOR_EVENT_FAN_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
            xTimerStart(fanTimer, portMAX_DELAY);
        }
    } else if (strcmp(event_base, BUTTON_EVENT) == 0) {
        if (event_id == BUTTON_EVENT_FAN_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
            xTimerStart(fanTimer, portMAX_DELAY);
        }
    } else if (strcmp(event_base, PARAMETERS_EVENT) == 0) {
        if (event_id == PARAMETERS_EVENT_STABLE) {
            ESP_ERROR_CHECK(turn_off());
        } else if (event_id == PARAMETERS_EVENT_UNSTABLE) {
            ESP_ERROR_CHECK(turn_on());
        }
    }
}

/**
 * @brief Turns on the fan.
 *
 * Updates the fan state, sets the GPIO level, and generates an event.
 */
esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (!fanOn) {
        fanOn = true;
        gpio_set_level(FAN_GPIO, HIGH_LEVEL);
        ComposterParameters_SetFanState(&composterParameters, fanOn);
        return esp_event_post(FAN_EVENT, FAN_EVENT_ON, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

/**
 * @brief Turns off the fan.
 *
 * Updates the fan state, sets the GPIO level, and generates an event.
 */
esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (fanOn) {
        fanOn = false;
        gpio_set_level(FAN_GPIO, LOW_LEVEL);
        ComposterParameters_SetFanState(&composterParameters, fanOn);
        return esp_event_post(FAN_EVENT, FAN_EVENT_OFF, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

/**
 * @brief Timer callback function for the fan.
 *
 * Turns off the fan if it is on and the composting parameters are stable.
 */
static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (fanOn
        && ComposterParameters_GetHumidityState(&composterParameters)
        && ComposterParameters_GetTemperatureState(&composterParameters)) {
        ESP_ERROR_CHECK(turn_off());
        xTimerStop(fanTimer, portMAX_DELAY);
    }
}
