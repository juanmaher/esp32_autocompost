#include "sensors/temperature_sensor.h"

#define TIMER_EXPIRED_BIT               (1 << 0)
#define STABLE_TEMPERATURE_TIMER       6000
#define UNSTABLE_TEMPERATURE_TIMER      1000
#define MAX_TEMPERATURE                 30


static const char *TAG = "AC_TemperatureSensor"; 

static TemperatureSensor_t sensor;

static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
}

static void reader_task(void *pvParameter) {
    esp_err_t err;
    EventBits_t uxBits;
    float temperature;

    while (1) {
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);

        if (uxBits & TIMER_EXPIRED_BIT) {
            err = ds18b20_set_resolution(sensor.handle, NULL, DS18B20_RESOLUTION_12B);
            if (err != ESP_OK) {
                continue;
            }

            err = ds18b20_trigger_temperature_conversion(sensor.handle, NULL);
            if (err != ESP_OK) {
                continue;
            }

            vTaskDelay(pdMS_TO_TICKS(800));

            err = ds18b20_get_temperature(sensor.handle, sensor.device_rom_id, &temperature);
            if (err != ESP_OK) {
                continue;
            }
            ESP_LOGI(TAG, "Temperature: %.2fC", temperature);

            if (temperature > MAX_TEMPERATURE) {
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(UNSTABLE_TEMPERATURE_TIMER), portMAX_DELAY);
            } else {
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(STABLE_TEMPERATURE_TIMER), portMAX_DELAY);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_ERROR_CHECK(onewire_del_bus(sensor.handle));
    ESP_LOGI(TAG, "1-wire bus deleted");

    vTaskDelete(NULL);
}

void TemperatureSensor_Start() {

    sensor.config = (onewire_rmt_config_t){
        .gpio_pin = TEMPERATURE_SENSOR_GPIO,
        .max_rx_bytes = 10,
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&sensor.config, &sensor.handle));
    ESP_LOGI(TAG, "1-wire bus installed");

    ESP_ERROR_CHECK(onewire_rom_search_context_create(sensor.handle, &sensor.context_handler));

    esp_err_t search_result = onewire_rom_search(sensor.context_handler);

    if (search_result == ESP_ERR_INVALID_CRC) {
        //continue;
    } else if (search_result == ESP_FAIL || search_result == ESP_ERR_NOT_FOUND) {
        //break;
    }

    ESP_ERROR_CHECK(onewire_rom_get_number(sensor.context_handler, sensor.device_rom_id));
    ESP_LOGI(TAG, "found device with rom id " ONEWIRE_ROM_ID_STR, ONEWIRE_ROM_ID(sensor.device_rom_id));

    ESP_ERROR_CHECK(onewire_rom_search_context_delete(sensor.context_handler));

    sensor.eventGroup = xEventGroupCreate(); 
    sensor.stableTimer = xTimerCreate("TemperatureSensor_Timer", pdMS_TO_TICKS(STABLE_TEMPERATURE_TIMER), true, NULL, timer_callback);

    xTaskCreate(reader_task, "TemperatureSensor_ReaderTask", 2048, NULL, 5, NULL);
    xTimerStart(sensor.stableTimer, 0);
}