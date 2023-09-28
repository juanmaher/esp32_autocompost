#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/events.h"
#include "actuators/mixer.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(MIXER_EVENT);

static const char *TAG = "AC_Mixer";

static esp_err_t turn_on();
static esp_err_t turn_off();

void Mixer_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
}

esp_err_t turn_on() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    return esp_event_post(MIXER_EVENT, MIXER_EVENT_ON, NULL, 0, portMAX_DELAY);
}

esp_err_t turn_off() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    return esp_event_post(MIXER_EVENT, MIXER_EVENT_OFF, NULL, 0, portMAX_DELAY);
}
