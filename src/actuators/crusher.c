#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/events.h"
#include "actuators/crusher.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(CRUSHER_EVENT);

static const char *TAG = "AC_Crusher";

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
static esp_err_t turn_on();
static esp_err_t turn_off();

void Crusher_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, CRUSHER_EVENT) == 0) {
        if (event_id == CRUSHER_EVENT_MANUAL_ON) {
            ESP_ERROR_CHECK(turn_on());
        } else if (event_id == CRUSHER_EVENT_MANUAL_OFF) {
            ESP_ERROR_CHECK(turn_off());
        }
    }
}

esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    return esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_ON, NULL, 0, portMAX_DELAY);
}

esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    return esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_OFF, NULL, 0, portMAX_DELAY);
}