/**
 * @file crusher.c
 * @brief Implementation of a controller for the crusher in a composting system.
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
#include "common/events.h"
#include "common/composter_parameters.h"
#include "common/gpios.h"
#include "actuators/crusher.h"

#define DEBUG false

#define START_CRUSHER_TIMER_MS 2 * 60 * 1000

// Definition of events related to the crusher
ESP_EVENT_DEFINE_BASE(CRUSHER_EVENT);

// Tag to identify log messages
static const char *TAG = "AC_Crusher";

// Variable to store the current state of the crusher (on/off)
static bool crusherOn;

// Timer handler for the crusher
static TimerHandle_t crusherTimer = NULL;

// External reference to composting parameters
extern ComposterParameters composterParameters;

// Declaration of internal functions
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();
static void timer_callback_function(TimerHandle_t xTimer);

/**
 * @brief Initializes the crusher controller.
 *
 * Configures the crusher GPIO and registers necessary event handlers.
 */
void Crusher_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    crusherOn = false;

    // Configuration of the crusher GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CRUSHER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // Creation of the crusher timer
    crusherTimer = xTimerCreate("CrusherTimer", pdMS_TO_TICKS(START_CRUSHER_TIMER_MS), pdTRUE, NULL, timer_callback_function);

    // Registration of event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(LOCK_EVENT, LOCK_EVENT_CRUSHER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(LID_EVENT, LID_EVENT_OPENED, &event_handler, NULL));
}

/**
 * @brief Event handler for the crusher.
 *
 * Handles events related to locking and lid opening.
 * Turns on or off the crusher as needed.
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, LOCK_EVENT) == 0) {
        if (event_id == LOCK_EVENT_CRUSHER_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
        }
    } else if (strcmp(event_base, LID_EVENT) == 0) {
        if (event_id == LID_EVENT_OPENED) {
            ESP_ERROR_CHECK(turn_off());
        }
    }
}

/**
 * @brief Turns on the crusher.
 *
 * Checks the lock state before turning on the crusher.
 * Generates an event and updates the crusher state.
 */
esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (!crusherOn) {
        if (ComposterParameters_GetLockState(&composterParameters)) {
            crusherOn = true;
            gpio_set_level(CRUSHER_GPIO, HIGH_LEVEL);
            xTimerStart(crusherTimer, portMAX_DELAY);
            ComposterParameters_SetCrusherState(&composterParameters, crusherOn);
            return esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_ON, NULL, 0, portMAX_DELAY);
        } else {
            ESP_LOGI(TAG, "Composter is locked, cannot turn on the crusher");
        }
    }
    return ESP_OK;
}

/**
 * @brief Turns off the crusher.
 *
 * Generates an event and updates the crusher state.
 */
esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (crusherOn) {
        crusherOn = false;
        gpio_set_level(CRUSHER_GPIO, LOW_LEVEL);
        xTimerStop(crusherTimer, portMAX_DELAY);
        ComposterParameters_SetCrusherState(&composterParameters, crusherOn);
        return esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_OFF, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

/**
 * @brief Timer callback function for the crusher.
 *
 * Turns off the crusher if it is on when the timer's time limit is reached.
 */
static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (crusherOn) {
        ESP_ERROR_CHECK(turn_off());
    }
}
