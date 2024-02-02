/**
 * @file lid_sensor.c
 * @brief Lid Sensor Implementation
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#include "common/composter_parameters.h"
#include "common/gpios.h"
#include "common/events.h"
#include "sensors/lid_sensor.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(LID_EVENT);

#define GPIO_INPUT_IO_0         LID_SENSOR_GPIO
#define GPIO_INPUT_PIN_SEL      (1ULL << GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT   0
#define LID_OPENED_TIMEOUT_MS   2 * 60 * 1000 /* 1200000 ms */

static const char *TAG = "AC_LidSensor";

static QueueHandle_t gpio_evt_queue = NULL;
static TimerHandle_t lidTimer = NULL;
extern ComposterParameters composterParameters;

static int current_gpio_state;

static void lid_sensor_task(void* arg);
static void timer_callback_function(TimerHandle_t xTimer);

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/**
 * @brief Callback function for Lid Sensor timer.
 * @param xTimer Timer handle.
 */
static void timer_callback_function(TimerHandle_t xTimer) {
    if (ComposterParameters_GetLidState(&composterParameters)) {
        ESP_ERROR_CHECK(esp_event_post(LID_EVENT, LID_EVENT_REQUEST_TO_CLOSE_LID, NULL, 0, portMAX_DELAY));
    }
}

/**
 * @brief Task to handle Lid Sensor events.
 * @param arg Task parameters (unused).
 */
static void lid_sensor_task(void* arg) {
    uint32_t io_num;
    while (true) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (current_gpio_state != gpio_get_level(io_num)) {
                current_gpio_state = gpio_get_level(io_num);
                if (DEBUG) printf("%s: GPIO[%"PRIu32"] intr, val: %d\n", TAG, io_num, gpio_get_level(io_num));
                if (gpio_get_level(io_num)) {
                    if (DEBUG) printf("LID OPENED\n");
                    ComposterParameters_SetLidState(&composterParameters, false);
                    ESP_ERROR_CHECK(esp_event_post(LID_EVENT, LID_EVENT_OPENED, NULL, 0, portMAX_DELAY));
                    xTimerStart(lidTimer, portMAX_DELAY);
                } else {
                    if (DEBUG) printf("LID CLOSED\n");
                    ComposterParameters_SetLidState(&composterParameters, true);
                    ESP_ERROR_CHECK(esp_event_post(LID_EVENT, LID_EVENT_CLOSED, NULL, 0, portMAX_DELAY));
                    xTimerStop(lidTimer, portMAX_DELAY);
                }
            }
        }
    }
}

void LidSensor_Start() {
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // Interrupt of any edge.
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    // Bit mask of the pins, use GPIO34 here.
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode.
    io_conf.mode = GPIO_MODE_INPUT;
    // Disable pull_up mode.
    io_conf.pull_up_en = 0;
    // Disable pull_down mode.
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    current_gpio_state = gpio_get_level(GPIO_INPUT_IO_0);
    if (current_gpio_state) {
        ComposterParameters_SetLidState(&composterParameters, true);
    } else {
        ComposterParameters_SetLidState(&composterParameters, false);
    }

    // Create a queue to handle gpio event from isr.
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // Start gpio task.
    xTaskCreate(lid_sensor_task, "lid_sensor_task", 2048, NULL, 3, NULL);

    // Install gpio isr service.
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook isr handler for specific gpio pin.
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

    lidTimer = xTimerCreate("lidTimer", pdMS_TO_TICKS(LID_OPENED_TIMEOUT_MS), pdTRUE, NULL, timer_callback_function);
}
