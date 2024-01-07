#ifndef COMPOSTER_PARAMETERS_H
#define COMPOSTER_PARAMETERS_H

/**
 * @file composter_parameters.h
 * @brief Declarations for the ComposterParameters module.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Define the structure to hold composting parameters
typedef struct {
    double complete;               // Represents completeness of the composting process
    int days;                      // Number of days the composting process has been ongoing
    double humidity;               // Humidity level in the composting system
    bool isHumidityStable;         // Flag indicating whether humidity is stable
    double temperature;            // Temperature in the composting system
    bool isTemperatureStable;      // Flag indicating whether temperature is stable
    bool mixer;                    // State flag for the mixer component
    bool crusher;                  // State flag for the crusher component
    bool fan;                      // State flag for the fan component
    bool lock;                     // State flag for the lock component
    bool lid;                      // State flag for the lid component
    SemaphoreHandle_t mutex;       // Semaphore for ensuring thread safety
} ComposterParameters;

// Function to initialize ComposterParameters structure
void ComposterParameters_Init(ComposterParameters *params);

// Functions to get various parameters and states
double ComposterParameters_GetComplete(const ComposterParameters* params);
int ComposterParameters_GetDays(const ComposterParameters* params);
double ComposterParameters_GetHumidity(const ComposterParameters* params);
bool ComposterParameters_GetHumidityState(const ComposterParameters* params);
double ComposterParameters_GetTemperature(const ComposterParameters* params);
bool ComposterParameters_GetTemperatureState(const ComposterParameters* params);
bool ComposterParameters_GetMixerState(const ComposterParameters* params);
bool ComposterParameters_GetCrusherState(const ComposterParameters* params);
bool ComposterParameters_GetFanState(const ComposterParameters* params);
bool ComposterParameters_GetLockState(const ComposterParameters* params);
bool ComposterParameters_GetLidState(const ComposterParameters* params);

// Functions to set various parameters and states
void ComposterParameters_SetComplete(ComposterParameters* params, double value);
void ComposterParameters_SetDays(ComposterParameters* params, int value);
void ComposterParameters_SetHumidity(ComposterParameters* params, double value);
void ComposterParameters_SetHumidityState(ComposterParameters* params, bool value);
void ComposterParameters_SetTemperature(ComposterParameters* params, double value);
void ComposterParameters_SetTemperatureState(ComposterParameters* params, bool value);
void ComposterParameters_SetMixerState(ComposterParameters* params, bool value);
void ComposterParameters_SetCrusherState(ComposterParameters* params, bool value);
void ComposterParameters_SetFanState(ComposterParameters* params, bool value);
void ComposterParameters_SetLockState(ComposterParameters* params, bool value);
void ComposterParameters_SetLidState(ComposterParameters* params, bool value);

#endif // COMPOSTER_PARAMETERS_H
