/**
 * @file temperature_sensor.c
 * @brief Temperature Sensor Implementation
 */
#include "common/composter_parameters.h"
#include "common/events.h"
#include "sensors/temperature_sensor.h"

#define DEBUG false

ESP_EVENT_DEFINE_BASE(TEMPERATURE_EVENT);

#define TIMER_EXPIRED_BIT               (1 << 0)
#define STABLE_HUMIDITY_TIMER_MS        10 * 60 * 1000
#define UNSTABLE_HUMIDITY_TIMER_MS      2 * 60 * 1000
#define ERROR_READ_SENSOR_TIMER_MS      60 * 1000
#define MAX_TEMPERATURE                 30

static const char *TAG = "AC_TemperatureSensor";

static TemperatureSensor_t sensor;
extern ComposterParameters composterParameters;

static int sensor_failures = 0;

static void timer_callback(TimerHandle_t pxTimer);
static void reader_task(void *pvParameter);
esp_err_t initialize_onewire_sensor();
int read_temperature_sensor();
void reset_temperature_sensor();

/**
 * @brief Callback function for the temperature sensor timer.
 * @param pxTimer Timer handle.
 */
static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
}

/**
 * @brief Task to handle temperature sensor reading.
 * @param pvParameter Task parameters (unused).
 */
static void reader_task(void *pvParameter) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;

    read_temperature_sensor();

    while (true) {
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);

        if (uxBits & TIMER_EXPIRED_BIT) {
            read_temperature_sensor();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Clean up resources when the task is deleted.
    ESP_ERROR_CHECK(onewire_del_bus(sensor.handle));
    if (DEBUG) ESP_LOGI(TAG, "1-wire bus deleted");

    vTaskDelete(NULL);
}

void TemperatureSensor_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Initialize the 1-Wire sensor bus.
    ESP_ERROR_CHECK(initialize_onewire_sensor());
    sensor.eventGroup = xEventGroupCreate(); 
    sensor.stableTimer = xTimerCreate("TemperatureSensor_Timer", pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), true, NULL, timer_callback);

    // Start the temperature sensor reader task.
    xTaskCreate(reader_task, "TemperatureSensor_ReaderTask", 2048, NULL, 5, NULL);
    xTimerStart(sensor.stableTimer, 0);
}

/**
 * @brief Initialize the 1-Wire sensor bus.
 * @return ESP_OK on success, else an error code.
 */
esp_err_t initialize_onewire_sensor() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Configuration for the 1-Wire sensor bus.
    sensor.config = (onewire_rmt_config_t){
        .gpio_pin = TEMPERATURE_SENSOR_GPIO,
        .max_rx_bytes = 10,
    };

    // Initialize the 1-Wire sensor bus with RMT.
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&sensor.config, &sensor.handle));
    if (DEBUG) ESP_LOGI(TAG, "1-wire bus installed");

    // Create a context for 1-Wire ROM search.
    ESP_ERROR_CHECK(onewire_rom_search_context_create(sensor.handle, &sensor.context_handler));

    // Perform a ROM search on the 1-Wire bus.
    esp_err_t search_result = onewire_rom_search(sensor.context_handler);

    if (search_result == ESP_ERR_INVALID_CRC) {
        // Continue;
    } else if (search_result == ESP_FAIL || search_result == ESP_ERR_NOT_FOUND) {
        // Break;
    }

    // Get the ROM number of the discovered device.
    ESP_ERROR_CHECK(onewire_rom_get_number(sensor.context_handler, sensor.device_rom_id));
    if (DEBUG) ESP_LOGI(TAG, "Found device with ROM ID " ONEWIRE_ROM_ID_STR, ONEWIRE_ROM_ID(sensor.device_rom_id));

    // Clean up the context after the ROM search.
    ESP_ERROR_CHECK(onewire_rom_search_context_delete(sensor.context_handler));

    return ESP_OK;
}


/**
 * @brief Reset the temperature sensor by reinitializing the 1-Wire sensor bus.
 */
void reset_temperature_sensor() {
    // Reset the temperature sensor by reinitializing the 1-Wire sensor bus.
    ESP_ERROR_CHECK(initialize_onewire_sensor());
    sensor_failures = 0;
}

/**
 * @brief Read the temperature from the sensor.
 * @return ESP_OK on success, else an error code.
 */
int read_temperature_sensor() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    esp_err_t err;
    float temperature;

    // Set the resolution of the DS18B20 sensor.
    err = ds18b20_set_resolution(sensor.handle, NULL, DS18B20_RESOLUTION_12B);
    if (err != ESP_OK) {
        goto error;
    }

    // Trigger temperature conversion on the DS18B20 sensor.
    err = ds18b20_trigger_temperature_conversion(sensor.handle, NULL);
    if (err != ESP_OK) {
        goto error;
    }

    // Wait for the DS18B20 sensor to complete the conversion.
    vTaskDelay(pdMS_TO_TICKS(800));

    // Read the temperature from the DS18B20 sensor.
    err = ds18b20_get_temperature(sensor.handle, sensor.device_rom_id, &temperature);
    if (err != ESP_OK) {
        goto error;
    }

    if (DEBUG) ESP_LOGI(TAG, "Temperature: %.2fC", temperature);

    // Set the temperature parameter in the ComposterParameters.
    ComposterParameters_SetTemperature(&composterParameters, temperature);

    // Check for unstable temperature conditions.
    if (temperature > MAX_TEMPERATURE) {
        ComposterParameters_SetTemperatureState(&composterParameters, false);
        esp_event_post(TEMPERATURE_EVENT, TEMPERATURE_EVENT_UNSTABLE, NULL, 0, portMAX_DELAY);
        xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(UNSTABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
    } else {
        ComposterParameters_SetTemperatureState(&composterParameters, true);
        esp_event_post(TEMPERATURE_EVENT, TEMPERATURE_EVENT_STABLE, NULL, 0, portMAX_DELAY);
        xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
    }

    return ESP_OK;

error:
    // Handle errors and reset the sensor on multiple failures.
    if (err != ESP_OK) {
        sensor_failures++;
        if (sensor_failures >= 5) {
            reset_temperature_sensor();
            xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(ERROR_READ_SENSOR_TIMER_MS), portMAX_DELAY);
        }
    }
    
    return err;
}