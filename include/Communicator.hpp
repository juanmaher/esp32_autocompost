#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

/* Firebase includes */
#include <value.h>
#include <json.h>
#include <app.h>
#include <rtdb.h>
#include "firebase_config.h"

using namespace ESPFirebase;

static const int MIXER_STATE_BIT = BIT0;
static const int CRUSHER_STATE_BIT = BIT1;
static const int TIME_STATE_BIT = BIT2;

// Class to communicate with cloud server
class Communicator {
    public:
        Communicator();
        void start();

    private:
        RTDB db;
        void configureFirebaseConnection();
        Json::Value createFirebaseComposter(std::string path);
        void updateParametersValues(TimerHandle_t xTimer);
};

#endif // COMMUNICATOR_H