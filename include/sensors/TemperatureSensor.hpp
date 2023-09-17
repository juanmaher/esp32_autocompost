#ifndef TEMPERATURESENSOR_HPP
#define TEMPERATURESENSOR_HPP

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

/* Libraries includes */
#include "esp_event.h"
#include "esp_log.h"

/* Internal includes */
#include "drivers/onewire_bus.h"
#include "drivers/ds18b20.h"

class TemperatureSensor {
    public:
        TemperatureSensor();
        void start();

    private:

};

#endif // TEMPERATURESENSOR
