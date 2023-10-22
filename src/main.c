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
    ESP_ERROR_CHECK(init_nvs());
    /* This loop is global to the whole program */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ComposterParameters_Init(&composterParameters);

    /* Start HMI */
    Buttons_Start();
    Display_Start();

    /* Start Sensors*/
    HumiditySensor_Start();
    TemperatureSensor_Start();
    LidSensor_Start();
    CapacitySensor_Start();
    
    /* Start communication modules */
    Wifi_Start();
    Communicator_Start();

    /* Start actuators */
    Lock_Start();
    Mixer_Start();
    Crusher_Start();
    Fan_Start();

    while(true);
    return 0;
}

esp_err_t init_nvs() {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}
