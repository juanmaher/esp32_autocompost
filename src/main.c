#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#include "common/composter_parameters.h"
#include "communication/communicator.h"
#include "communication/wifi.h"

ComposterParameters composterParameters;

int app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    /* This loop is global to the whole program */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ComposterParameters_Init(&composterParameters);

    Wifi_start();
    Communicator_start();

    while(true);
    return 0;
}

