#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "common/composter_parameters.h"
#include "common/events.h"
#include "common/gpios.h"
#include "actuators/lock.h"
#include "sensors/capacity_sensor.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(LOCK_EVENT);

static const char *TAG = "AC_Lock";
static bool lockOn;

extern ComposterParameters composterParameters;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t lock();
static esp_err_t unlock();
static void force_unlock();

void Lock_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    lockOn = false;

    ESP_ERROR_CHECK(esp_event_handler_register(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_CRUSHER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(BUTTON_EVENT, BUTTON_EVENT_CRUSHER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, CRUSHER_EVENT_OFF, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CAPACITY_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(LID_EVENT, LID_EVENT_OPENED, &event_handler, NULL));
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, COMMUNICATOR_EVENT) == 0) {
        if (event_id == COMMUNICATOR_EVENT_CRUSHER_MANUAL_ON) {
            if (!lock()) {
                ESP_ERROR_CHECK(esp_event_post(LOCK_EVENT, LOCK_EVENT_CRUSHER_MANUAL_ON, NULL, 0, portMAX_DELAY));
            }
        }
    } else if (strcmp(event_base, BUTTON_EVENT) == 0) {
        if (event_id == BUTTON_EVENT_CRUSHER_MANUAL_ON) {
            if (!lock()) {
                ESP_ERROR_CHECK(esp_event_post(LOCK_EVENT, LOCK_EVENT_CRUSHER_MANUAL_ON, NULL, 0, portMAX_DELAY));
            }
        }
    } else if (strcmp(event_base, CRUSHER_EVENT) == 0) {
        if (event_id == CRUSHER_EVENT_OFF) {
            unlock();
        }
    } else if (strcmp(event_base, CAPACITY_EVENT) == 0) {
        if (event_id == CAPACITY_EVENT_EMPTY || event_id == CAPACITY_EVENT_PARTIAL) {
            unlock();
        } else if (event_id == CAPACITY_EVENT_FULL) {
            lock();
        }
    } else if (strcmp(event_base, LID_EVENT) == 0) {
        if (event_id == LID_EVENT_OPENED) {
            force_unlock();
        }
    }
}

static esp_err_t lock() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (!lockOn) {
        if (ComposterParameters_GetLidState(&composterParameters)) {
            lockOn = true;
            ESP_LOGI(TAG, "Lock lid");
            gpio_set_level(LOCK_GPIO, HIGH_LEVEL);
            ComposterParameters_SetLockState(&composterParameters, lockOn);
            return ESP_OK;
        } else {
            ESP_LOGI(TAG, "Lid is opened cannot lock composter");
            ESP_ERROR_CHECK(esp_event_post(LOCK_EVENT, LOCK_EVENT_REQUEST_TO_CLOSE_LID, NULL, 0, portMAX_DELAY));
        }
    }

    return ESP_FAIL;
}

static esp_err_t unlock() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (lockOn) {
        if (ComposterParameters_GetComplete(&composterParameters) < MAX_CAPACITY_PERCENT || 
                ComposterParameters_GetCrusherState(&composterParameters)) {
            lockOn = false;
            ESP_LOGI(TAG, "Unlock lid");
            gpio_set_level(LOCK_GPIO, LOW_LEVEL);
            ComposterParameters_SetLockState(&composterParameters, lockOn);
            return ESP_OK;
        } else {
            ESP_LOGI(TAG, "Composter is full! Cannot unlock composter");
            ESP_ERROR_CHECK(esp_event_post(LOCK_EVENT, LOCK_EVENT_REQUEST_TO_EMPTY_COMPOSTER, NULL, 0, portMAX_DELAY));
        }
    }

    return ESP_FAIL;
}

static void force_unlock() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (lockOn) {
        lockOn = false;
        ESP_LOGI(TAG, "Force unlock lid");
        gpio_set_level(LOCK_GPIO, LOW_LEVEL);
        ComposterParameters_SetLockState(&composterParameters, lockOn);
    }
}