#ifndef CAPACITYSENSOR_H
#define CAPACITYSENSOR_H

/**
 * @file capacity_sensor.h
 * @brief Declarations for the capacity module.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "driver/mcpwm_cap.h"
#include "driver/gpio.h"
#include "common/gpios.h"

#define MAX_CAPACITY_FLOAT                10.0
#define MAX_CAPACITY_PERCENT              0.1
#define MIN_CAPACITY_FLOAT                30.0
#define MIN_CAPACITY_PERCENT              0.9

typedef struct {
    mcpwm_cap_channel_handle_t cap_chan;
    mcpwm_capture_event_callbacks_t cbs;
    TaskHandle_t task_to_notify;
    mcpwm_cap_timer_handle_t cap_timer;
    mcpwm_capture_timer_config_t cap_conf;
    mcpwm_capture_channel_config_t cap_ch_conf;
    gpio_config_t io_conf;
    TimerHandle_t fullTimer;
    EventGroupHandle_t eventGroup;
    int timerId;
} CapacitySensor_t;

/**
 * @brief Initializes the capacity module.
 */
void CapacitySensor_Start();

#endif // CAPACITYSENSOR_H