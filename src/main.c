#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#include "common/composter_parameters.h"
#include "hmi/buttons.h"
#include "hmi/display.h"
#include "communication/communicator.h"
#include "communication/wifi.h"
#include "actuators/crusher.h"
#include "actuators/mixer.h"
#include "actuators/fan.h"

#include "sensors/humidity_sensor.h"
#include "sensors/temperature_sensor.h"
#include "sensors/lid_sensor.h"

ComposterParameters composterParameters;

int app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    /* This loop is global to the whole program */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ComposterParameters_Init(&composterParameters);

    /* Start HMI */
    Buttons_Start();
    Display_Start();

    /* Start Sensors*/
    /*HumiditySensor_Start();
    TemperatureSensor_Start();*/
    LidSensor_Start();

    /* Start communication modules */
    Wifi_Start();
    Communicator_Start();

    /* Start actuators */
    Mixer_Start();
    Crusher_Start();
    Fan_Start();

    while(true);
    return 0;
}

