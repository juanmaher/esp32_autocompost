#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/composter_parameters.h"
#include "common/events.h"
#include "sensors/capacity_sensor.h"

#define DEBUG true

#define TIMER_EXPIRED_BIT           BIT0
#define MIXER_OFF_BIT               BIT1
#define FULL_CAPACITY_TIMER_MS      1 * 60 * 1000

ESP_EVENT_DEFINE_BASE(CAPACITY_EVENT);

#define SENSOR_TRIG_GPIO    HC_SR04_TRIG_GPIO
#define SENSOR_ECHO_GPIO    HC_SR04_ECHO_GPIO

const static char *TAG = "AC_CapacitySensor";

static CapacitySensor_t sensor;
extern ComposterParameters composterParameters;

typedef enum {
    EMPTY = 0,
    PARTIAL,
    FULL,
    STATES
} capacity_state_t;

static capacity_state_t current_capacity_state = EMPTY;

static void timer_callback(TimerHandle_t pxTimer);
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static bool hc_sr04_echo_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data);
static void gen_trig_output(void);
static void capacity_measurement_task(void *pvParameters);

static void timer_callback(TimerHandle_t pxTimer) {
    // Check if mixer is on
    if (ComposterParameters_GetMixerState(&composterParameters)) {
        xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, MIXER_EVENT) == 0) {
        if (event_id == MIXER_EVENT_OFF) {
            xEventGroupSetBits(sensor.eventGroup, MIXER_OFF_BIT);
        }
    }
}

static bool hc_sr04_echo_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data) {
    static uint32_t cap_val_begin_of_sample = 0;
    static uint32_t cap_val_end_of_sample = 0;
    TaskHandle_t task_to_notify = (TaskHandle_t)user_data;
    BaseType_t high_task_wakeup = pdFALSE;

    //calculate the interval in the ISR,
    //so that the interval will be always correct even when capture_queue is not handled in time and overflow.
    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        // store the timestamp when pos edge is detected
        cap_val_begin_of_sample = edata->cap_value;
        cap_val_end_of_sample = cap_val_begin_of_sample;
    } else {
        cap_val_end_of_sample = edata->cap_value;
        uint32_t tof_ticks = cap_val_end_of_sample - cap_val_begin_of_sample;

        // notify the task to calculate the distance
        xTaskNotifyFromISR(task_to_notify, tof_ticks, eSetValueWithOverwrite, &high_task_wakeup);
    }

    return high_task_wakeup == pdTRUE;
}

static void gen_trig_output(void) {
    gpio_set_level(SENSOR_TRIG_GPIO, HIGH_LEVEL);
    esp_rom_delay_us(10);
    gpio_set_level(SENSOR_TRIG_GPIO, LOW_LEVEL);
}

static void capacity_measurement_task(void *pvParameters) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;
    capacity_state_t prev_capacity_state = current_capacity_state;
    float capacity_value;

    if (DEBUG) ESP_LOGI(TAG, "Register capture callback");
    sensor.task_to_notify = xTaskGetCurrentTaskHandle();
    sensor.cbs.on_cap = hc_sr04_echo_callback;
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(sensor.cap_chan, &sensor.cbs, sensor.task_to_notify));

    if (DEBUG) ESP_LOGI(TAG, "Enable capture channel");
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(sensor.cap_chan));

    if (DEBUG) ESP_LOGI(TAG, "Enable and start capture timer");
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(sensor.cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(sensor.cap_timer));

    uint32_t tof_ticks;
    while (true) {
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT | MIXER_OFF_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

        if (uxBits & TIMER_EXPIRED_BIT || uxBits & MIXER_OFF_BIT) {
            gen_trig_output();
            if (xTaskNotifyWait(0x00, ULONG_MAX, &tof_ticks, pdMS_TO_TICKS(1000)) == pdTRUE) {
                float pulse_width_us = tof_ticks * (1000000.0 / esp_clk_apb_freq());
                if (pulse_width_us > 35000) {
                    continue;
                }
                capacity_value = (float)pulse_width_us / 58; // Modificar para calcular el valor de la capacidad
                if (DEBUG) ESP_LOGI(TAG, "Measured capacity: %.2f", capacity_value);

                if (capacity_value < MAX_CAPACITY_FLOAT) {
                    current_capacity_state = FULL;
                } else if (capacity_value > MIN_CAPACITY_FLOAT) {
                    current_capacity_state = EMPTY;
                } else {
                    current_capacity_state = PARTIAL;
                }

                if (prev_capacity_state != current_capacity_state) {
                    prev_capacity_state = current_capacity_state;
                    switch (current_capacity_state) {
                        case EMPTY:
                            xTimerStop(sensor.fullTimer, 0);
                            esp_event_post(CAPACITY_EVENT, CAPACITY_EVENT_EMPTY, NULL, 0, portMAX_DELAY);
                            break;
                        case PARTIAL:
                            xTimerStop(sensor.fullTimer, 0);
                            esp_event_post(CAPACITY_EVENT, CAPACITY_EVENT_PARTIAL, NULL, 0, portMAX_DELAY);
                            break;
                        case FULL:
                            xTimerStart(sensor.fullTimer, 0);
                            esp_event_post(CAPACITY_EVENT, CAPACITY_EVENT_FULL, NULL, 0, portMAX_DELAY);
                            break;
                        default:
                            break;
                    }
                }

                float percentage = 100 * capacity_value / 32;

                if (percentage < 0) {
                    percentage = 0;
                } else if (percentage > 100) {
                    percentage = 100;
                }

                ComposterParameters_SetComplete(&composterParameters, percentage);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void CapacitySensor_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    sensor.cap_timer = NULL;

    sensor.cap_conf = (mcpwm_capture_timer_config_t){
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };

    sensor.cap_ch_conf = (mcpwm_capture_channel_config_t){
        .gpio_num = SENSOR_ECHO_GPIO,
        .prescale = 1,
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        .flags.pull_up = true,
    };

    sensor.io_conf = (gpio_config_t){
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << SENSOR_TRIG_GPIO,
    };

    if (DEBUG) ESP_LOGI(TAG, "Install capture timer");
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&sensor.cap_conf, &sensor.cap_timer));

    if (DEBUG) ESP_LOGI(TAG, "Install capture channel");
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(sensor.cap_timer, &sensor.cap_ch_conf, &sensor.cap_chan));

    if (DEBUG) ESP_LOGI(TAG, "Configure Trig pin");
    ESP_ERROR_CHECK(gpio_config(&sensor.io_conf));
    ESP_ERROR_CHECK(gpio_set_level(SENSOR_TRIG_GPIO, 0));

    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, MIXER_EVENT_OFF, &event_handler, NULL));

    sensor.eventGroup = xEventGroupCreate(); 
    sensor.fullTimer = xTimerCreate("CapacitySensor_Timer", pdMS_TO_TICKS(FULL_CAPACITY_TIMER_MS), true, NULL, timer_callback);

    xTaskCreate(capacity_measurement_task, "capacity_task", 4096, NULL, 5, NULL);
}

