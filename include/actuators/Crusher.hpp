#ifndef CRUSHER_HPP
#define CRUSHER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "common/events.h"

class Crusher {
    public:
        Crusher();
        void start();

    private:
        void turnOn();
        void turnOff();

};

#endif // CRUSHER_HPP
