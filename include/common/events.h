#ifndef EVENTS_H
#define EVENTS_H

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

// Definición de eventos de la trituradora
ESP_EVENT_DECLARE_BASE(CRUSHER_EVENT);

typedef enum {
    CRUSHER_EVENT_ON,
    CRUSHER_EVENT_OFF
} CrusherEvent_t;

// Definición de eventos de la mezcladora
ESP_EVENT_DECLARE_BASE(MIXER_EVENT);

typedef enum {
    MIXER_EVENT_ON,
    MIXER_EVENT_OFF
} MixerEvent_t;

// Definición de eventos del ventilador
ESP_EVENT_DECLARE_BASE(FAN_EVENT);

typedef enum {
    FAN_EVENT_ON,
    FAN_EVENT_OFF
} FanEvent_t;

// Definición de eventos de la traba
ESP_EVENT_DECLARE_BASE(LOCK_EVENT);

typedef enum {
    LOCK_EVENT_ON,
    LOCK_EVENT_OFF,
    LOCK_EVENT_CRUSHER_MANUAL_ON,
    LOCK_EVENT_REQUEST_TO_CLOSE_LID,
    LOCK_EVENT_REQUEST_TO_EMPTY_COMPOSTER
} LockEvent_t;

// Definición de eventos del sensor de tapa
ESP_EVENT_DECLARE_BASE(LID_EVENT);

typedef enum {
    LID_EVENT_OPENED,
    LID_EVENT_CLOSED,
    LID_EVENT_REQUEST_TO_CLOSE_LID
} LidEvent_t;

// Definición de eventos de Communicator
ESP_EVENT_DECLARE_BASE(COMMUNICATOR_EVENT);

typedef enum {
    COMMUNICATOR_EVENT_MIXER_MANUAL_ON,
    COMMUNICATOR_EVENT_CRUSHER_MANUAL_ON,
    COMMUNICATOR_EVENT_FAN_MANUAL_ON
} CommunicatorEvent_t;

// Definición de eventos de Buttons
ESP_EVENT_DECLARE_BASE(BUTTON_EVENT);

typedef enum {
    BUTTON_EVENT_MIXER_MANUAL_ON,
    BUTTON_EVENT_CRUSHER_MANUAL_ON,
    BUTTON_EVENT_FAN_MANUAL_ON
} ButtonEvent_t;

// Definición de eventos de WIFI
ESP_EVENT_DECLARE_BASE(WIFI_EVENT_INTERNAL);

typedef enum {
    WIFI_EVENT_CONNECTION_ON,
    WIFI_EVENT_CONNECTION_OFF
} WifiEvent_t;

// Definición de eventos de parameters
ESP_EVENT_DECLARE_BASE(PARAMETERS_EVENT);

typedef enum {
    PARAMETERS_EVENT_STABLE,
    PARAMETERS_EVENT_UNSTABLE
} ParametersEvent_t;

// Definición de eventos de humedad
ESP_EVENT_DECLARE_BASE(HUMIDITY_EVENT);

typedef enum {
    HUMIDITY_EVENT_STABLE,
    HUMIDITY_EVENT_UNSTABLE
} HumidityEvent_t;

// Definición de eventos de temperatura
ESP_EVENT_DECLARE_BASE(TEMPERATURE_EVENT);

typedef enum {
    TEMPERATURE_EVENT_STABLE,
    TEMPERATURE_EVENT_UNSTABLE
} TemperatureEvent_t;

// Definición de eventos de capacidad
ESP_EVENT_DECLARE_BASE(CAPACITY_EVENT);

typedef enum {
    CAPACITY_EVENT_EMPTY,
    CAPACITY_EVENT_PARTIAL,
    CAPACITY_EVENT_FULL
} CapaciityEvent_t;

#ifdef __cplusplus
}
#endif

#endif // EVENTS_H