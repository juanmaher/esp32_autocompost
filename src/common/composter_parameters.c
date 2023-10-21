#include "common/composter_parameters.h"
#include "freertos/semphr.h"

void ComposterParameters_Init(ComposterParameters *params) {
    if (params == NULL) {
        return;
    }

    params->complete = 0.0;
    params->days = 0;
    params->humidity = 0.0;
    params->temperature = 0.0;
    params->mixer = false;
    params->crusher = false;
    params->fan = false;
    params->lock = false;
    params->lid = false;
    params->mutex = xSemaphoreCreateMutex();
}

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

void ComposterParameters_SetTemperature(ComposterParameters* params, double value) {
    if (params == NULL || params->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(params->mutex, portMAX_DELAY) == pdTRUE) {
        params->temperature = value;
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