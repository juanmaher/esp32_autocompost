#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/events.h"
#include "common/composter_parameters.h"
#include "actuators/mixer.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(MIXER_EVENT);

static const char *TAG = "AC_Mixer";
static bool mixerOn;

static TimerHandle_t mixerTimer = NULL;
extern ComposterParameters composterParameters;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();

static void timer_callback_function(TimerHandle_t xTimer);

void Mixer_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    mixerOn = false;

    mixerTimer = xTimerCreate("MixerTimer", pdMS_TO_TICKS(10000), pdTRUE, NULL, timer_callback_function);

    ESP_ERROR_CHECK(esp_event_handler_register(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_MIXER_MANUAL_ON, &event_handler, NULL));
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, COMMUNICATOR_EVENT) == 0) {
        if (event_id == COMMUNICATOR_EVENT_MIXER_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
        }
    }
}

esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (!mixerOn) {
        mixerOn = true;
        xTimerStart(mixerTimer, portMAX_DELAY);
        ComposterParameters_SetMixerState(&composterParameters, mixerOn);
        return esp_event_post(MIXER_EVENT, MIXER_EVENT_ON, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (mixerOn) {
        mixerOn = false;
        xTimerStop(mixerTimer, portMAX_DELAY);
        ComposterParameters_SetMixerState(&composterParameters, mixerOn);
        return esp_event_post(MIXER_EVENT, MIXER_EVENT_OFF, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (mixerOn) {
        ESP_ERROR_CHECK(turn_off());
    } else {
        ESP_ERROR_CHECK(turn_on());
    }
}
