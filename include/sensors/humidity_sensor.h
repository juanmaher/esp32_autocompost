#ifndef HUMIDITYSENSOR_H
#define HUMIDITYSENSOR_H

/**
 * @file humidity_sensor.h
 * @brief Declarations for the humidity sensor module.
 */

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

/* Libraries includes */
#include "esp_event.h"
#include "esp_log.h"

/* Internal includes */
#include "common/events.h"
#include "common/gpios.h"
#include "drivers/DHT22.h"
#include "driver/gpio.h"

typedef struct {
    TimerHandle_t stableTimer;
    EventGroupHandle_t eventGroup;
    int timerId;
} HumiditySensor_t;

/**
 * @brief Initializes the humidity sensor module.
 * 
 */
void HumiditySensor_Start();

#endif // HUMIDITYSENSOR_H