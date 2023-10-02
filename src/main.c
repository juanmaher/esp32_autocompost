
#include "nvs_flash.h"
// #include "communication/Communicator.hpp"
// #include "communication/Wifi.hpp"
// #include "actuators/Mixer.hpp"
// #include "actuators/Crusher.hpp"
#include "sensors/CapactitySensor.h"
#include "sensors/HumiditySensor.h"
#include "sensors/TemperatureSensor.h"

HumiditySensor_t humiditySensor;
TemperatureSensor_t temperatureSensor;
CapacitySensor_t capacitySensor;


int app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    // This loop is global to the whole program 
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //Wifi wifi = Wifi();

    // Communicator communicator = Communicator();
    // communicator.start();

    // Mixer mixer = Mixer();
    // mixer.start();

    // Crusher crusher = Crusher();  
    // crusher.start();

    // Declarar e inicializar una instancia de HumiditySensor_t
    HumiditySensor_Init(&humiditySensor);
    HumiditySensor_Start(&humiditySensor);

    TemperatureSensor_Init(&temperatureSensor);
    TemperatureSensor_Start(&temperatureSensor);

    CapacitySensor_Init(&capacitySensor);
    CapacitySensor_Start(&capacitySensor);

    while(true);
    return 0;
}



