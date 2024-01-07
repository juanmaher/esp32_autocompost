/**
 * @file main.c
 * @brief Main Application Entry Point
 */
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#include "common/composter_parameters.h"
#include "hmi/buttons.h"
#include "hmi/display.h"
#include "communication/communicator.h"
#include "communication/wifi.h"
#include "actuators/lock.h"
#include "actuators/crusher.h"
#include "actuators/mixer.h"
#include "actuators/fan.h"
#include "sensors/humidity_sensor.h"
#include "sensors/temperature_sensor.h"
#include "sensors/capacity_sensor.h"
#include "sensors/lid_sensor.h"

ComposterParameters composterParameters;

esp_err_t init_nvs();

int app_main(void)
{
    // Initialize NVS to store persistent data.
    ESP_ERROR_CHECK(init_nvs());

    // Create the default event loop.
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize and set default values for ComposterParameters.
    ComposterParameters_Init(&composterParameters);

    // Start Human-Machine Interface (HMI) components.
    Buttons_Start();
    Display_Start();

    // Start sensor modules.
    HumiditySensor_Start();
    TemperatureSensor_Start();
    LidSensor_Start();
    CapacitySensor_Start();

    // Start communication modules.
    Wifi_Start();
    Communicator_Start();

    // Start actuator modules.
    Lock_Start();
    Mixer_Start();
    Crusher_Start();
    Fan_Start();

    // Main loop to keep the program running.
    while (true);

    return 0;
}

/**
 * @brief Initialize the Non-Volatile Storage (NVS).
 * @return ESP_OK on success, else an error code.
 */
esp_err_t init_nvs() {
    // Initialize NVS flash memory.
    esp_err_t ret = nvs_flash_init();

    // Handle cases where there are no free pages or a new version is found.
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase NVS flash and reinitialize.
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}
