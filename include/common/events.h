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
    CRUSHER_EVENT_OFF,
    CRUSHER_EVENT_MANUAL_ON,
    CRUSHER_EVENT_MANUAL_OFF
} CrusherEvent_t;

// Definición de eventos de la mezcladora
ESP_EVENT_DECLARE_BASE(MIXER_EVENT);

typedef enum {
    MIXER_EVENT_ON,
    MIXER_EVENT_OFF,
    MIXER_EVENT_MANUAL_ON,
    MIXER_EVENT_MANUAL_OFF
} MixerEvent_t;

typedef enum {
    WIFI_EVENT_CONNECTION_ON,
    WIFI_EVENT_CONNECTION_OFF
} WifiEvent_t;

#ifdef __cplusplus
}
#endif

#endif // EVENTS_H