#include "common/composter_parameters.h"
#include "common/events.h"
#include "sensors/temperature_sensor.h"

#define DEBUG true

#define TIMER_EXPIRED_BIT               (1 << 0)
#define STABLE_HUMIDITY_TIMER_MS        10 * 60 * 1000
#define UNSTABLE_HUMIDITY_TIMER_MS      2 * 60 * 1000
#define MAX_TEMPERATURE                 30

ESP_EVENT_DEFINE_BASE(TEMPERATURE_EVENT);

static const char *TAG = "AC_TemperatureSensor";

static TemperatureSensor_t sensor;
extern ComposterParameters composterParameters;

static int sensor_failures = 0;

static void timer_callback(TimerHandle_t pxTimer);
static void reader_task(void *pvParameter);
esp_err_t initialize_onewire_sensor();
void reset_temperature_sensor();

static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
}

static void reader_task(void *pvParameter) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    esp_err_t err;
    EventBits_t uxBits;
    float temperature;

    while (true) {
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);

        if (uxBits & TIMER_EXPIRED_BIT) {
            err = ds18b20_set_resolution(sensor.handle, NULL, DS18B20_RESOLUTION_12B);
            if (err != ESP_OK) {
                goto error;
            }

            err = ds18b20_trigger_temperature_conversion(sensor.handle, NULL);
            if (err != ESP_OK) {
                goto error;
            }

            vTaskDelay(pdMS_TO_TICKS(800));

            err = ds18b20_get_temperature(sensor.handle, sensor.device_rom_id, &temperature);
            if (err != ESP_OK) {
                goto error;
            }

            if (DEBUG) ESP_LOGI(TAG, "Temperature: %.2fC", temperature);

            ComposterParameters_SetTemperature(&composterParameters, temperature);

            if (temperature > MAX_TEMPERATURE) {
                ComposterParameters_SetTemperatureState(&composterParameters, false);
                esp_event_post(TEMPERATURE_EVENT, TEMPERATURE_EVENT_UNSTABLE, NULL, 0, portMAX_DELAY);
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(UNSTABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
            } else {
                ComposterParameters_SetTemperatureState(&composterParameters, true);
                esp_event_post(TEMPERATURE_EVENT, TEMPERATURE_EVENT_STABLE, NULL, 0, portMAX_DELAY);
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
            }

            continue;

            error:
                if (err != ESP_OK) {
                    sensor_failures++;
                    if (sensor_failures >= 5) {
                        reset_temperature_sensor();
                    }
                }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_ERROR_CHECK(onewire_del_bus(sensor.handle));
    if (DEBUG) ESP_LOGI(TAG, "1-wire bus deleted");

    vTaskDelete(NULL);
}

void TemperatureSensor_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    ESP_ERROR_CHECK(initialize_onewire_sensor());
    sensor.eventGroup = xEventGroupCreate(); 
    sensor.stableTimer = xTimerCreate("TemperatureSensor_Timer", pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), true, NULL, timer_callback);

    xTaskCreate(reader_task, "TemperatureSensor_ReaderTask", 2048, NULL, 5, NULL);
    xTimerStart(sensor.stableTimer, 0);
}

esp_err_t initialize_onewire_sensor() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    sensor.config = (onewire_rmt_config_t){
        .gpio_pin = TEMPERATURE_SENSOR_GPIO,
        .max_rx_bytes = 10,
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&sensor.config, &sensor.handle));
    if (DEBUG) ESP_LOGI(TAG, "1-wire bus installed");

    ESP_ERROR_CHECK(onewire_rom_search_context_create(sensor.handle, &sensor.context_handler));

    esp_err_t search_result = onewire_rom_search(sensor.context_handler);

    if (search_result == ESP_ERR_INVALID_CRC) {
        //continue;
    } else if (search_result == ESP_FAIL || search_result == ESP_ERR_NOT_FOUND) {
        //break;
    }

    ESP_ERROR_CHECK(onewire_rom_get_number(sensor.context_handler, sensor.device_rom_id));
    if (DEBUG) ESP_LOGI(TAG, "found device with rom id " ONEWIRE_ROM_ID_STR, ONEWIRE_ROM_ID(sensor.device_rom_id));

    ESP_ERROR_CHECK(onewire_rom_search_context_delete(sensor.context_handler));

    return ESP_OK;
}

void reset_temperature_sensor() {
    ESP_ERROR_CHECK(initialize_onewire_sensor());
    sensor_failures = 0;
}