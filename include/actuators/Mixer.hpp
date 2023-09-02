#ifndef MIXER_HPP
#define MIXER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"

// Definición de eventos de la mezcladora
ESP_EVENT_DECLARE_BASE(MIXER_EVENT);

typedef enum {
    MIXER_EVENT_ON,
    MIXER_EVENT_OFF
} MixerEvent_t;

class Mixer {
    public:
        Mixer();
        void start();

    private:
        void turnOn();
        void turnOff();
};

#endif // MIXER_HPP
