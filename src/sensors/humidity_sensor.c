/**
 * @file humidity_sensor.c
 * @brief Implementation of the humidity sensor module.
 */
#include "common/composter_parameters.h"
#include "common/events.h"
#include "sensors/humidity_sensor.h"

#define DEBUG false

#define TIMER_EXPIRED_BIT               (1 << 0)
#define STABLE_HUMIDITY_TIMER_MS        10 * 60 * 1000
#define UNSTABLE_HUMIDITY_TIMER_MS      2 * 60 * 1000
#define ERROR_READ_SENSOR_TIMER_MS      60 * 1000
#define MAX_HUMIDITY                    60

ESP_EVENT_DEFINE_BASE(HUMIDITY_EVENT);

static const char *TAG = "AC_HumiditySensor";

static HumiditySensor_t sensor;
extern ComposterParameters composterParameters;

static int sensor_failures = 0;

static void timer_callback(TimerHandle_t pxTimer);
static void reader_task(void *pvParameter);
int read_humidity_sensor();
void reset_humidity_sensor();

/**
 * @brief Callback function for the humidity sensor timer.
 * @param pxTimer Timer handle.
 */
static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
}

/**
 * @brief Task for reading humidity sensor values.
 * @param pvParameter Task parameters (unused).
 */
static void reader_task(void *pvParameter) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;

    // Initial reading of the humidity sensor
    read_humidity_sensor();

    while (true) {
        // Wait for the timer expiration event
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);

        // Perform a new reading when the timer expires
        if (uxBits & TIMER_EXPIRED_BIT) {
            read_humidity_sensor();
        }

        // Delay the task by 1000 milliseconds
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // This line will not be reached, as the task runs in an infinite loop
    vTaskDelete(NULL);
}

void HumiditySensor_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Create an event group and a stable timer for the humidity sensor
    sensor.eventGroup = xEventGroupCreate(); 
    sensor.stableTimer = xTimerCreate("HumiditySensor_Timer", pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), true, NULL, timer_callback);

    // Set the GPIO pin for the humidity sensor
    setDHTgpio(HUMIDITY_SENSOR_GPIO);

    // Create and start the reader task
    xTaskCreate(reader_task, "HumiditySensor_ReaderTask", 2048, NULL, 4, NULL);
    xTimerStart(sensor.stableTimer, 0);
}

/**
 * @brief Reset the humidity sensor configuration.
 */
void reset_humidity_sensor() {
    // Reset the GPIO pin for the humidity sensor and reset failure count
    setDHTgpio(HUMIDITY_SENSOR_GPIO);
    sensor_failures = 0;
}

/**
 * @brief Read values from the humidity sensor.
 * @return Result of the sensor reading operation.
 */
int read_humidity_sensor() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (DEBUG) ESP_LOGI(TAG, "Reading values:");
    int ret = readDHT();

    if (ret != DHT_OK) {
        // Handle sensor reading errors and track failures
        errorHandler(ret);
        sensor_failures++;

        // If failures exceed a threshold, reset the sensor and adjust timer period
        if (sensor_failures >= 5) {
            reset_humidity_sensor();
            xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(ERROR_READ_SENSOR_TIMER_MS), portMAX_DELAY);
        }

        return ret;
    }

    // Read humidity value from the sensor
    float humidity = getHumidity();
    if (DEBUG) ESP_LOGI(TAG, "Humidity %.2f %%", humidity);

    // Update ComposterParameters with the humidity value
    ComposterParameters_SetHumidity(&composterParameters, humidity);

    // Check humidity state and trigger events accordingly
    if (humidity > MAX_HUMIDITY) {
        ComposterParameters_SetHumidityState(&composterParameters, false);
        esp_event_post(HUMIDITY_EVENT, HUMIDITY_EVENT_UNSTABLE, NULL, 0, portMAX_DELAY);
        xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(UNSTABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
    } else {
        ComposterParameters_SetHumidityState(&composterParameters, true);
        esp_event_post(HUMIDITY_EVENT, HUMIDITY_EVENT_STABLE, NULL, 0, portMAX_DELAY);
        xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
    }

    return ret;
}
