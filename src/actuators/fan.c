#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/events.h"
#include "actuators/fan.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(FAN_EVENT);

static const char *TAG = "AC_Fan";
static bool fanOn;

static TimerHandle_t fanTimer = NULL;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();
static void timer_callback_function(TimerHandle_t xTimer);

void Fan_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    fanOn = false;

    fanTimer = xTimerCreate("FanTimer", pdMS_TO_TICKS(10000), pdTRUE, NULL, timer_callback_function);
    xTimerStart(fanTimer, portMAX_DELAY);

    ESP_ERROR_CHECK(esp_event_handler_register(FAN_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, FAN_EVENT) == 0) {
        if (event_id == FAN_EVENT_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
        } else if (event_id == FAN_EVENT_MANUAL_OFF) {
            ESP_ERROR_CHECK(turn_off());
        }
    }
}

esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    fanOn = true;
    return esp_event_post(FAN_EVENT, FAN_EVENT_ON, NULL, 0, portMAX_DELAY);
}

esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    fanOn = false;
    return esp_event_post(FAN_EVENT, FAN_EVENT_OFF, NULL, 0, portMAX_DELAY);
}

static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (fanOn) {
        ESP_ERROR_CHECK(turn_off());
    } else {
        ESP_ERROR_CHECK(turn_on());
    }
}