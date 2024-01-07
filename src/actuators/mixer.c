/**
 * @file mixer.c
 * @brief Implementation of a controller for the compost mixer in a composting system.
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
#include "actuators/mixer.h"

#define DEBUG false

// Definitions for timer durations
#define RUTINE_MIXING_TIMER_MS      6 * 60 * 60 * 1000 /* 21600000 ms */
#define START_MIXER_TIMER_MS        2 * 60 * 1000

// Definition of events related to the mixer
ESP_EVENT_DEFINE_BASE(MIXER_EVENT);

// Tag to identify log messages
static const char *TAG = "AC_Mixer";

// Variable to store the current state of the mixer (on/off)
static bool mixerOn;

// Timer handles for routine mixing and starting the mixer
static TimerHandle_t startMixerTimer = NULL;
static TimerHandle_t rutineMixingTimer = NULL;

// External reference to composting parameters
extern ComposterParameters composterParameters;

// Declaration of internal functions
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();
static void start_mixer_timer_callback(TimerHandle_t xTimer);
static void rutine_mixing_timer_callback(TimerHandle_t xTimer);

/**
 * @brief Initializes the mixer controller.
 *
 * Configures the mixer GPIO, creates timers, and registers necessary event handlers
 * for manual control, button presses, parameter changes, and crusher events.
 */
void Mixer_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    mixerOn = false;

    // Configuration of the mixer GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MIXER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // Creation of timers
    rutineMixingTimer = xTimerCreate("rutineMixingTimer", pdMS_TO_TICKS(RUTINE_MIXING_TIMER_MS), pdTRUE, NULL, rutine_mixing_timer_callback);
    startMixerTimer = xTimerCreate("startMixerTimer", pdMS_TO_TICKS(START_MIXER_TIMER_MS), pdTRUE, NULL, start_mixer_timer_callback);

    // Registration of event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_MIXER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(BUTTON_EVENT, BUTTON_EVENT_MIXER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PARAMETERS_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, CRUSHER_EVENT_ON, &event_handler, NULL));

    // Start the routine mixing timer
    xTimerStart(rutineMixingTimer, portMAX_DELAY);
}

/**
 * @brief Event handler for the mixer.
 *
 * Handles events related to manual mixer control, button presses, parameter changes,
 * and crusher events. Turns the mixer on or off accordingly.
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, COMMUNICATOR_EVENT) == 0) {
        if (event_id == COMMUNICATOR_EVENT_MIXER_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
            xTimerStart(startMixerTimer, portMAX_DELAY);
        }
    } else if (strcmp(event_base, BUTTON_EVENT) == 0) {
        if (event_id == BUTTON_EVENT_MIXER_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
            xTimerStart(startMixerTimer, portMAX_DELAY);
        }
    } else if (strcmp(event_base, PARAMETERS_EVENT) == 0) {
        if (event_id == PARAMETERS_EVENT_STABLE) {
            ESP_ERROR_CHECK(turn_off());
        } else if (event_id == PARAMETERS_EVENT_UNSTABLE) {
            ESP_ERROR_CHECK(turn_on());
        }
    } else if (strcmp(event_base, CRUSHER_EVENT) == 0) {
        if (event_id == CRUSHER_EVENT_ON) {
            ESP_ERROR_CHECK(turn_on());
            xTimerStart(startMixerTimer, portMAX_DELAY);
        }
    }
}

/**
 * @brief Turns on the compost mixer.
 *
 * Checks if the mixer is not already on before turning it on.
 * Generates an event and updates the parameter state accordingly.
 */
esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (!mixerOn) {
        mixerOn = true;
        gpio_set_level(MIXER_GPIO, HIGH_LEVEL);
        ComposterParameters_SetMixerState(&composterParameters, mixerOn);
        return esp_event_post(MIXER_EVENT, MIXER_EVENT_ON, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

/**
 * @brief Turns off the compost mixer.
 *
 * Checks if the mixer is not already off before turning it off.
 * Generates an event and updates the parameter state accordingly.
 */
esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (mixerOn) {
        mixerOn = false;
        gpio_set_level(MIXER_GPIO, LOW_LEVEL);
        ComposterParameters_SetMixerState(&composterParameters, mixerOn);
        return esp_event_post(MIXER_EVENT, MIXER_EVENT_OFF, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

/**
 * @brief Timer callback function for starting the mixer.
 *
 * Checks conditions before starting the mixer.
 * Stops the timer if the mixer is started.
 */
static void start_mixer_timer_callback(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (mixerOn
            && ComposterParameters_GetHumidityState(&composterParameters)
            && ComposterParameters_GetTemperatureState(&composterParameters)) {
        ESP_ERROR_CHECK(turn_off());
        xTimerStop(startMixerTimer, portMAX_DELAY);
    }
}

/**
 * @brief Timer callback function for routine mixing.
 *
 * Turns on the mixer at regular intervals for routine mixing.
 */
static void rutine_mixing_timer_callback(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    ESP_ERROR_CHECK(turn_on());
    xTimerStart(startMixerTimer, portMAX_DELAY);
}
