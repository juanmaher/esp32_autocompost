#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/composter_parameters.h"
#include "common/events.h"
#include "common/gpios.h"
#include "actuators/mixer.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(MIXER_EVENT);

static const char *TAG = "AC_Mixer";
static bool mixerOn;

static TimerHandle_t startMixerTimer = NULL;
static TimerHandle_t rutineMixingTimer = NULL;
extern ComposterParameters composterParameters;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();

static void start_mixer_timer_callback(TimerHandle_t xTimer);
static void rutine_mixing_timer_callback(TimerHandle_t xTimer);

void Mixer_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    mixerOn = false;

    /* 6 hours = 6 * 60 * 60 * 1000 = 21600000 ms*/
    rutineMixingTimer = xTimerCreate("rutineMixingTimer", pdMS_TO_TICKS(21600000), pdTRUE, NULL, rutine_mixing_timer_callback);
    startMixerTimer = xTimerCreate("startMixerTimer", pdMS_TO_TICKS(2000), pdTRUE, NULL, start_mixer_timer_callback);

    ESP_ERROR_CHECK(esp_event_handler_register(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_MIXER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PARAMETERS_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, CRUSHER_EVENT_ON, &event_handler, NULL));

    xTimerStart(rutineMixingTimer, portMAX_DELAY);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, COMMUNICATOR_EVENT) == 0) {
        if (event_id == COMMUNICATOR_EVENT_MIXER_MANUAL_ON) {
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

static void start_mixer_timer_callback(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (mixerOn
            && ComposterParameters_GetHumidityState(&composterParameters)
            && ComposterParameters_GetTemperatureState(&composterParameters)) {
        ESP_ERROR_CHECK(turn_off());
        xTimerStop(startMixerTimer, portMAX_DELAY);
    }
}

static void rutine_mixing_timer_callback(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    ESP_ERROR_CHECK(turn_on());
    xTimerStart(startMixerTimer, portMAX_DELAY);
}