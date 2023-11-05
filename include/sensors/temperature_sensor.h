#ifndef TEMPERATURESENSOR_H
#define TEMPERATURESENSOR_H

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

/* Libraries includes */
#include "esp_event.h"
#include "esp_log.h"

/* Internal includes */
#include "drivers/onewire_bus.h"
#include "drivers/ds18b20.h"
#include "common/gpios.h"

typedef struct {
    TimerHandle_t stableTimer;
    EventGroupHandle_t eventGroup;
    onewire_rmt_config_t config;
    onewire_bus_handle_t handle;
    onewire_rom_search_context_handler_t context_handler;
    uint8_t device_rom_id[8];
} TemperatureSensor_t;

void TemperatureSensor_Start();

#endif // TEMPERATURESENSOR_H