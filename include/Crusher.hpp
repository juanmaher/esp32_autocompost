#ifndef CRUSHER_HPP
#define CRUSHER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"

// Definición de eventos de la trituradora
ESP_EVENT_DECLARE_BASE(CRUSHER_EVENT);

typedef enum {
    CRUSHER_EVENT_ON,
    CRUSHER_EVENT_OFF,
    CRUSHER_EVENT_MANUAL_ON,
    CRUSHER_EVENT_MANUAL_OFF
} CrusherEvent_t;

class Crusher {
    public:
        Crusher();
        void start();

    private:
        void turnOn();
        void turnOff();

};

#endif // CRUSHER_HPP
