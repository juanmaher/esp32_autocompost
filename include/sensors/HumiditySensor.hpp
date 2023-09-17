#ifndef HUMIDITYSENSOR_HPP
#define HUMIDITYSENSOR_HPP

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

/* Libraries includes */
#include "esp_event.h"
#include "esp_log.h"

/* Internal includes */
#include "common/events.h"
#include "drivers/DHT22.hpp"
#include "driver/gpio.h"

class HumiditySensor {
    public:
        HumiditySensor();
        void start();

    private:

};

#endif // HUMIDITYSENSOR
