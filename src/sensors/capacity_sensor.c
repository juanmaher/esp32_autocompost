#include "sensors/capacity_sensor.h"

const static char *TAG = "AC_CapacitySensor";

static CapacitySensor_t sensor;

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
    gpio_set_level(HC_SR04_TRIG_GPIO, 1); // set high0
    esp_rom_delay_us(10);
    gpio_set_level(HC_SR04_TRIG_GPIO, 0); // set low
}

static void capacity_measurement_task(void *pvParameters) {
    ESP_LOGI(TAG, "Register capture callback");
    sensor.task_to_notify = xTaskGetCurrentTaskHandle();
    sensor.cbs.on_cap = hc_sr04_echo_callback;
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(sensor.cap_chan, &sensor.cbs, sensor.task_to_notify));

    ESP_LOGI(TAG, "Enable capture channel");
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(sensor.cap_chan));

    ESP_LOGI(TAG, "Enable and start capture timer");
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(sensor.cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(sensor.cap_timer));

    uint32_t tof_ticks;
    while (1) {
        gen_trig_output();
        if (xTaskNotifyWait(0x00, ULONG_MAX, &tof_ticks, pdMS_TO_TICKS(1000)) == pdTRUE) {
            float pulse_width_us = tof_ticks * (1000000.0 / esp_clk_apb_freq());
            if (pulse_width_us > 35000) {
                continue;
            }
            float capacity_value = (float)pulse_width_us / 58; // Modificar para calcular el valor de la capacidad
            ESP_LOGI(TAG, "Measured capacity: %.2f", capacity_value);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void CapacitySensor_Start() {
    sensor.cap_timer = NULL;

    sensor.cap_conf = (mcpwm_capture_timer_config_t){
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };

    sensor.cap_ch_conf = (mcpwm_capture_channel_config_t){
        .gpio_num = HC_SR04_ECHO_GPIO,
        .prescale = 1,
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        .flags.pull_up = true,
    };

    sensor.io_conf = (gpio_config_t){
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << HC_SR04_TRIG_GPIO,
    };

    ESP_LOGI(TAG, "Install capture timer");
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&sensor.cap_conf, &sensor.cap_timer));

    ESP_LOGI(TAG, "Install capture channel");
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(sensor.cap_timer, &sensor.cap_ch_conf, &sensor.cap_chan));

    ESP_LOGI(TAG, "Configure Trig pin");
    ESP_ERROR_CHECK(gpio_config(&sensor.io_conf));
    ESP_ERROR_CHECK(gpio_set_level(HC_SR04_TRIG_GPIO, 0));

    xTaskCreate(capacity_measurement_task, "capacity_task", 4096, NULL, 5, NULL);
}

