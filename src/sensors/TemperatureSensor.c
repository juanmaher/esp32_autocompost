#include "sensors/TemperatureSensor.h"

#define TEMPERATURE_SENSOR_GPIO_PIN        GPIO_NUM_5

static const char *TAG = "AC_TemperatureSensor"; 

static void reader_task(void *pvParameter) {
    TemperatureSensor_t *sensor = (TemperatureSensor_t *)pvParameter;

    while (1) {
        esp_err_t err;
        vTaskDelay(pdMS_TO_TICKS(200));

        err = ds18b20_set_resolution(sensor->handle, NULL, DS18B20_RESOLUTION_12B);
        if (err != ESP_OK) {
            continue;
        }

        err = ds18b20_trigger_temperature_conversion(sensor->handle, NULL);
        if (err != ESP_OK) {
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(800));

        float temperature;
        err = ds18b20_get_temperature(sensor->handle, sensor->device_rom_id, &temperature);
        if (err != ESP_OK) {
            continue;
        }
        ESP_LOGI(TAG, "temperature of device " ONEWIRE_ROM_ID_STR ": %.2fC", ONEWIRE_ROM_ID(sensor->device_rom_id), temperature);
    }

    ESP_ERROR_CHECK(onewire_del_bus(sensor->handle));
    ESP_LOGI(TAG, "1-wire bus deleted");

    vTaskDelete(NULL);
}

void TemperatureSensor_Init(TemperatureSensor_t *sensor) {
    sensor->config = (onewire_rmt_config_t){
        .gpio_pin = TEMPERATURE_SENSOR_GPIO_PIN,
        .max_rx_bytes = 10,
    };
}

void TemperatureSensor_Start(TemperatureSensor_t *sensor) {
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&sensor->config, &sensor->handle));
    ESP_LOGI(TAG, "1-wire bus installed");

    ESP_ERROR_CHECK(onewire_rom_search_context_create(sensor->handle, &sensor->context_handler));

    esp_err_t search_result = onewire_rom_search(sensor->context_handler);

    if (search_result == ESP_ERR_INVALID_CRC) {
        //continue;
    } else if (search_result == ESP_FAIL || search_result == ESP_ERR_NOT_FOUND) {
        //break;
    }

    ESP_ERROR_CHECK(onewire_rom_get_number(sensor->context_handler, sensor->device_rom_id));
    ESP_LOGI(TAG, "found device with rom id " ONEWIRE_ROM_ID_STR, ONEWIRE_ROM_ID(sensor->device_rom_id));

    ESP_ERROR_CHECK(onewire_rom_search_context_delete(sensor->context_handler));

    xTaskCreate(reader_task, "TemperatureSensor_ReaderTask", 2048, sensor, 5, NULL);
}