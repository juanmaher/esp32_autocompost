#include "actuators/Mixer.hpp"

Mixer::Mixer() {
    // Constructor de la clase Mixer
}

void Mixer::start() {
    
}

void Mixer::turnOn() {
    // Prender mixer
    esp_event_post(MIXER_EVENT, static_cast<int>(MIXER_EVENT_ON), nullptr, 0, portMAX_DELAY);
}

void Mixer::turnOff() {
    // Apagar mixer
    esp_event_post(MIXER_EVENT, static_cast<int>(MIXER_EVENT_OFF), nullptr, 0, portMAX_DELAY);
}
