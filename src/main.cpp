#include "nvs_flash.h"
#include "communication/Communicator.hpp"
#include "communication/Wifi.hpp"
#include "actuators/Mixer.hpp"
#include "actuators/Crusher.hpp"

extern "C" int app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    /* This loop is global to the whole program */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    Wifi wifi = Wifi();
    wifi.start();

    Communicator communicator = Communicator();
    communicator.start();

    Mixer mixer = Mixer();
    mixer.start();

    Crusher crusher = Crusher();  
    crusher.start();

    vTaskStartScheduler();

    while(true);
    return 0;
}

