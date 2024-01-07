/**
 * @file composter_parameters.c
 * @brief Implementation of functions related to composting system parameters.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Inclusion of FreeRTOS and ESP-IDF libraries
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Inclusion of custom header files
#include "common/composter_parameters.h"
#include "common/events.h"

#define DEBUG false

// Definition of the event base for parameters
ESP_EVENT_DEFINE_BASE(PARAMETERS_EVENT);

// Tag to identify log messages
static const char *TAG = "AC_Parameters";

// Variables to track stability of temperature and humidity
static bool isCurrentTemperatureStable = true;
static bool isCurrentHumidityStable = true;
static bool areParametersStable = true;

// Event handler function declaration
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * @brief Initializes the composting system parameters.
 *
 * Initializes the structure with default values and creates a mutex for thread safety.
 * Registers event handlers for temperature and humidity events.
 */
void ComposterParameters_Init(ComposterParameters *params) {
    if (params == NULL) {
        return;
    }

    // Initialization of parameters
    params->complete = 0.0;
    params->days = 0;
    params->humidity = 0.0;
    params->isHumidityStable = true;
    params->temperature = 0.0;
    params->isTemperatureStable = true;
    params->mixer = false;
    params->crusher = false;
    params->fan = false;
    params->lock = false;
    params->lid = false;

    // Creation of mutex for thread safety
    params->mutex = xSemaphoreCreateMutex();

    // Registration of event handlers for temperature and humidity events
    ESP_ERROR_CHECK(esp_event_handler_register(TEMPERATURE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(HUMIDITY_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
}

/**
 * @brief Event handler for temperature and humidity events.
 *
 * Handles events related to temperature and humidity stability changes.
 * Updates the overall parameter stability and generates events accordingly.
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, TEMPERATURE_EVENT) == 0) {
        if (event_id == TEMPERATURE_EVENT_STABLE) {
            if (DEBUG) ESP_LOGI(TAG, "Temperature stable event received");
            isCurrentTemperatureStable = true;
        } else if (event_id == TEMPERATURE_EVENT_UNSTABLE) {
            if (DEBUG) ESP_LOGI(TAG, "Temperature unstable event received");
            isCurrentTemperatureStable = false;
        }
    } else if (strcmp(event_base, HUMIDITY_EVENT) == 0) {
        if (event_id == HUMIDITY_EVENT_STABLE) {
            if (DEBUG) ESP_LOGI(TAG, "Humidity stable event received");
            isCurrentHumidityStable = true;
        } else if (event_id == HUMIDITY_EVENT_UNSTABLE) {
            if (DEBUG) ESP_LOGI(TAG, "Humidity unstable event received");
            isCurrentHumidityStable = false;
        }
    }

    // Check if both temperature and humidity are stable and update overall stability
    if (isCurrentTemperatureStable && isCurrentHumidityStable && !areParametersStable) {
        areParametersStable = true;
        esp_event_post(PARAMETERS_EVENT, PARAMETERS_EVENT_STABLE, NULL, 0, portMAX_DELAY);
    } else if ((!isCurrentTemperatureStable || !isCurrentHumidityStable) && areParametersStable) {
        areParametersStable = false;
        esp_event_post(PARAMETERS_EVENT, PARAMETERS_EVENT_UNSTABLE, NULL, 0, portMAX_DELAY);
    }
}

// Function implementations for getting parameters

double ComposterParameters_GetComplete(const ComposterParameters* params) {
    double result = 0.0;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->complete;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

int ComposterParameters_GetDays(const ComposterParameters* params) {
    int result = 0;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->days;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

double ComposterParameters_GetHumidity(const ComposterParameters* params) {
    double result = 0.0;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->humidity;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetHumidityState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->isHumidityStable;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

double ComposterParameters_GetTemperature(const ComposterParameters* params) {
    double result = 0.0;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->temperature;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetTemperatureState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->isTemperatureStable;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetMixerState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->mixer;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetCrusherState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->crusher;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetFanState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->fan;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetLockState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->lock;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

bool ComposterParameters_GetLidState(const ComposterParameters* params) {
    bool result = false;
    
    if (params == NULL || params->mutex == NULL) {
        return result;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        result = params->lid;
        xSemaphoreGive(params->mutex);
    }

    return result;
}

// Function implementations for setting parameters

void ComposterParameters_SetComplete(ComposterParameters* params, double value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->complete = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetDays(ComposterParameters* params, int value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->days = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetHumidity(ComposterParameters* params, double value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->humidity = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetHumidityState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->isHumidityStable = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetTemperature(ComposterParameters* params, double value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->temperature = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetTemperatureState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->isTemperatureStable = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetMixerState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->mixer = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetCrusherState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->crusher = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetFanState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->fan = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetLockState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->lock = value;
        xSemaphoreGive(params->mutex);
    }
}

void ComposterParameters_SetLidState(ComposterParameters* params, bool value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->lid = value;
        xSemaphoreGive(params->mutex);
    }
}
