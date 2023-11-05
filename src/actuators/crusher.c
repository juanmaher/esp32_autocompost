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
#include "actuators/crusher.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(CRUSHER_EVENT);

static const char *TAG = "AC_Crusher";
static bool crusherOn;

static TimerHandle_t crusherTimer = NULL;
extern ComposterParameters composterParameters;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();

static void timer_callback_function(TimerHandle_t xTimer);

void Crusher_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    crusherOn = false;

    crusherTimer = xTimerCreate("CrusherTimer", pdMS_TO_TICKS(10000), pdTRUE, NULL, timer_callback_function);

    ESP_ERROR_CHECK(esp_event_handler_register(LOCK_EVENT, LOCK_EVENT_CRUSHER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(LID_EVENT, LID_EVENT_OPENED, &event_handler, NULL));
}

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

esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (!crusherOn) {
        if (ComposterParameters_GetLockState(&composterParameters)) {
            crusherOn = true;
            xTimerStart(crusherTimer, portMAX_DELAY);
            ComposterParameters_SetCrusherState(&composterParameters, crusherOn);
            return esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_ON, NULL, 0, portMAX_DELAY);
        } else {
            ESP_LOGI(TAG, "Composter is locked cannot turn on crusher");
        }
    }
    return ESP_OK;
}

esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (crusherOn) {
        crusherOn = false;
        xTimerStop(crusherTimer, portMAX_DELAY);
        ComposterParameters_SetCrusherState(&composterParameters, crusherOn);
        return esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_OFF, NULL, 0, portMAX_DELAY);
    }
    return ESP_OK;
}

static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (crusherOn) {
        ESP_ERROR_CHECK(turn_off());
    }
}
