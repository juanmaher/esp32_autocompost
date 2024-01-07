#ifndef EVENTS_H
#define EVENTS_H

/**
 * @file events.h
 * @brief Declaration of the events used in the project
 */
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

// Definición de eventos de la trituradora
ESP_EVENT_DECLARE_BASE(CRUSHER_EVENT);

// Enumeration of events related to the crusher component
typedef enum {
    CRUSHER_EVENT_ON,   // Crusher turned on event
    CRUSHER_EVENT_OFF   // Crusher turned off event
} CrusherEvent_t;

// Definición de eventos de la mezcladora
ESP_EVENT_DECLARE_BASE(MIXER_EVENT);

// Enumeration of events related to the mixer component
typedef enum {
    MIXER_EVENT_ON,   // Mixer turned on event
    MIXER_EVENT_OFF   // Mixer turned off event
} MixerEvent_t;

// Definición de eventos del ventilador
ESP_EVENT_DECLARE_BASE(FAN_EVENT);

// Enumeration of events related to the fan component
typedef enum {
    FAN_EVENT_ON,   // Fan turned on event
    FAN_EVENT_OFF   // Fan turned off event
} FanEvent_t;

// Definición de eventos de la traba
ESP_EVENT_DECLARE_BASE(LOCK_EVENT);

// Enumeration of events related to the lock component
typedef enum {
    LOCK_EVENT_ON,                    // Lock engaged event
    LOCK_EVENT_OFF,                   // Lock disengaged event
    LOCK_EVENT_CRUSHER_MANUAL_ON,     // Manual crusher activation requested event
    LOCK_EVENT_REQUEST_TO_CLOSE_LID,  // Request to close lid event
    LOCK_EVENT_REQUEST_TO_EMPTY_COMPOSTER  // Request to empty composter event
} LockEvent_t;

// Definición de eventos del sensor de tapa
ESP_EVENT_DECLARE_BASE(LID_EVENT);

// Enumeration of events related to the lid sensor component
typedef enum {
    LID_EVENT_OPENED,              // Lid opened event
    LID_EVENT_CLOSED,              // Lid closed event
    LID_EVENT_REQUEST_TO_CLOSE_LID // Request to close lid event
} LidEvent_t;

// Definición de eventos de Communicator
ESP_EVENT_DECLARE_BASE(COMMUNICATOR_EVENT);

// Enumeration of events related to the communicator component
typedef enum {
    COMMUNICATOR_EVENT_MIXER_MANUAL_ON,   // Manual mixer activation requested event
    COMMUNICATOR_EVENT_CRUSHER_MANUAL_ON, // Manual crusher activation requested event
    COMMUNICATOR_EVENT_FAN_MANUAL_ON      // Manual fan activation requested event
} CommunicatorEvent_t;

// Definición de eventos de Buttons
ESP_EVENT_DECLARE_BASE(BUTTON_EVENT);

// Enumeration of events related to physical buttons
typedef enum {
    BUTTON_EVENT_MIXER_MANUAL_ON,   // Manual mixer activation requested via button event
    BUTTON_EVENT_CRUSHER_MANUAL_ON, // Manual crusher activation requested via button event
    BUTTON_EVENT_FAN_MANUAL_ON      // Manual fan activation requested via button event
} ButtonEvent_t;

// Definición de eventos de WIFI
ESP_EVENT_DECLARE_BASE(WIFI_EVENT_INTERNAL);

// Enumeration of events related to internal WiFi events
typedef enum {
    WIFI_EVENT_CONNECTION_ON,  // WiFi connection established event
    WIFI_EVENT_CONNECTION_OFF  // WiFi connection lost event
} WifiEvent_t;

// Definición de eventos de parameters
ESP_EVENT_DECLARE_BASE(PARAMETERS_EVENT);

// Enumeration of events related to system parameters
typedef enum {
    PARAMETERS_EVENT_STABLE,    // Parameters stable event
    PARAMETERS_EVENT_UNSTABLE   // Parameters unstable event
} ParametersEvent_t;

// Definición de eventos de humedad
ESP_EVENT_DECLARE_BASE(HUMIDITY_EVENT);

// Enumeration of events related to humidity
typedef enum {
    HUMIDITY_EVENT_STABLE,  // Humidity stable event
    HUMIDITY_EVENT_UNSTABLE // Humidity unstable event
} HumidityEvent_t;

// Definición de eventos de temperatura
ESP_EVENT_DECLARE_BASE(TEMPERATURE_EVENT);

// Enumeration of events related to temperature
typedef enum {
    TEMPERATURE_EVENT_STABLE,    // Temperature stable event
    TEMPERATURE_EVENT_UNSTABLE   // Temperature unstable event
} TemperatureEvent_t;

// Definición de eventos de capacidad
ESP_EVENT_DECLARE_BASE(CAPACITY_EVENT);

// Enumeration of events related to composting system capacity
typedef enum {
    CAPACITY_EVENT_NOT_FULL,  // Composter not full event
    CAPACITY_EVENT_FULL       // Composter full event
} CapaciityEvent_t;

#ifdef __cplusplus
}
#endif

#endif // EVENTS_H
