#ifndef CAPACITYSENSOR_H
#define CAPACITYSENSOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "driver/mcpwm_cap.h"
#include "driver/gpio.h"
#include "common/gpios.h"

typedef struct {
    mcpwm_cap_channel_handle_t cap_chan;
    mcpwm_capture_event_callbacks_t cbs;
    TaskHandle_t task_to_notify;
    mcpwm_cap_timer_handle_t cap_timer;
    mcpwm_capture_timer_config_t cap_conf;
    mcpwm_capture_channel_config_t cap_ch_conf;
    gpio_config_t io_conf;
} CapacitySensor_t;

void CapacitySensor_Start();

#endif // CAPACITYSENSOR_H