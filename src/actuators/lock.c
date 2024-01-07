/**
 * @file lock.c
 * @brief Implementation of a controller for the lock in a composting system.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Inclusion of FreeRTOS and ESP-IDF libraries
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

// Inclusion of custom header files
#include "common/composter_parameters.h"
#include "common/events.h"
#include "common/gpios.h"
#include "actuators/lock.h"
#include "sensors/capacity_sensor.h"

#define DEBUG false

// Definition of events related to the lock
ESP_EVENT_DEFINE_BASE(LOCK_EVENT);

// Tag to identify log messages
static const char *TAG = "AC_Lock";

// Variable to store the current state of the lock (locked/unlocked)
static bool lockOn;

// External reference to composting parameters
extern ComposterParameters composterParameters;

// Declaration of internal functions
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t lock();
static esp_err_t unlock();
static void force_unlock();

/**
 * @brief Initializes the lock controller.
 *
 * Configures the lock GPIO and registers necessary event handlers for manual control,
 * crusher events, capacity sensor events, and lid events.
 */
void Lock_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    lockOn = false;

    // Configuration of the lock GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LOCK_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // Registration of event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_CRUSHER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(BUTTON_EVENT, BUTTON_EVENT_CRUSHER_MANUAL_ON, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, CRUSHER_EVENT_OFF, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CAPACITY_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(LID_EVENT, LID_EVENT_OPENED, &event_handler, NULL));
}

/**
 * @brief Event handler for the lock.
 *
 * Handles events related to manual crusher control, button presses, crusher events,
 * capacity sensor events, and lid events. Locks or unlocks the composter accordingly.
 */
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
        if (event_id == CAPACITY_EVENT_NOT_FULL) {
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

/**
 * @brief Locks the composter lid.
 *
 * Checks if the lid is closed before locking the composter.
 * Generates an event if the lid is open and a request to close the lid is required.
 */
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
            ESP_LOGI(TAG, "Lid is opened, cannot lock composter");
            ESP_ERROR_CHECK(esp_event_post(LOCK_EVENT, LOCK_EVENT_REQUEST_TO_CLOSE_LID, NULL, 0, portMAX_DELAY));
        }
    }

    return ESP_FAIL;
}

/**
 * @brief Unlocks the composter lid.
 *
 * Checks conditions before unlocking the composter.
 * Generates an event if the composter is full or requires emptying.
 */
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

/**
 * @brief Forces the composter lid to unlock.
 *
 * Used when the lid is manually opened, ignoring the current conditions.
 */
static void force_unlock() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (lockOn) {
        lockOn = false;
        ESP_LOGI(TAG, "Force unlock lid");
        gpio_set_level(LOCK_GPIO, LOW_LEVEL);
        ComposterParameters_SetLockState(&composterParameters, lockOn);
    }
}
