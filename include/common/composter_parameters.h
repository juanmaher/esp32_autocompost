#ifndef COMPOSTER_PARAMETERS_H
#define COMPOSTER_PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    double complete;
    int days;
    double humidity;
    double temperature;
    bool mixer;
    bool crusher;
    bool fan;
    bool lock;
    bool lid;
    SemaphoreHandle_t mutex;
} ComposterParameters;

void ComposterParameters_Init(ComposterParameters *params);

double ComposterParameters_GetComplete(const ComposterParameters* params);
int ComposterParameters_GetDays(const ComposterParameters* params);
double ComposterParameters_GetHumidity(const ComposterParameters* params);
double ComposterParameters_GetTemperature(const ComposterParameters* params);
bool ComposterParameters_GetMixerState(const ComposterParameters* params);
bool ComposterParameters_GetCrusherState(const ComposterParameters* params);
bool ComposterParameters_GetFanState(const ComposterParameters* params);
bool ComposterParameters_GetLockState(const ComposterParameters* params);
bool ComposterParameters_GetLidState(const ComposterParameters* params);

void ComposterParameters_SetComplete(ComposterParameters* params, double value);
void ComposterParameters_SetDays(ComposterParameters* params, int value);
void ComposterParameters_SetHumidity(ComposterParameters* params, double value);
void ComposterParameters_SetTemperature(ComposterParameters* params, double value);
void ComposterParameters_SetMixerState(ComposterParameters* params, bool value);
void ComposterParameters_SetCrusherState(ComposterParameters* params, bool value);
void ComposterParameters_SetFanState(ComposterParameters* params, bool value);
void ComposterParameters_SetLockState(ComposterParameters* params, bool value);
void ComposterParameters_SetLidState(ComposterParameters* params, bool value);

#endif // COMPOSTER_PARAMETERS_H
