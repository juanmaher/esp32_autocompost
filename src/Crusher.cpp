#include "Crusher.hpp"

Crusher::Crusher() {
    // Constructor de la clase Crusher
}

void Crusher::start() {
    
}

void Crusher::turnOn() {
    // Prender crusher
    esp_event_post(CRUSHER_EVENT, static_cast<int>(CRUSHER_EVENT_ON), nullptr, 0, portMAX_DELAY);
}

void Crusher::turnOff() {
    // Apagar crusher
    esp_event_post(CRUSHER_EVENT, static_cast<int>(CRUSHER_EVENT_OFF), nullptr, 0, portMAX_DELAY);
}
