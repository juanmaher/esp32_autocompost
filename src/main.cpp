#include "nvs_flash.h"
#include "communication/Communicator.hpp"
#include "communication/Wifi.hpp"
#include "actuators/Mixer.hpp"
#include "actuators/Crusher.hpp"
#include "sensors/HumiditySensor.hpp"

extern "C" int app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    /* This loop is global to the whole program */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //Wifi wifi = Wifi();

    /*Communicator communicator = Communicator();
    communicator.start();

    Mixer mixer = Mixer();
    mixer.start();

    Crusher crusher = Crusher();  
    crusher.start();*/


    HumiditySensor humiditySensor = HumiditySensor();
    humiditySensor.start();


    while(true);
    return 0;
}

