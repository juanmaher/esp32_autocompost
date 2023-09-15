#ifndef MIXER_HPP
#define MIXER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "common/events.h"

class Mixer {
    public:
        Mixer();
        void start();

    private:
        void turnOn();
        void turnOff();
};

#endif // MIXER_HPP
