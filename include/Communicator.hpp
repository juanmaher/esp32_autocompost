#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <string.h>

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

/* Libraries includes */
#include "esp_log.h"
#include <value.h>
#include <json.h>
#include <app.h>
#include <rtdb.h>

/* Internal includes */
#include "ComposterParameters.hpp"
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
        static void updateSensorsParametersValues(TimerHandle_t xTimer, RTDB& db, ComposterParameters& composterParameters);
        static Json::Value getFirebaseComposterData(RTDB& db);
        RTDB db;
        ComposterParameters composterParameters;

    private:
        void configureFirebaseConnection();
        static Json::Value createFirebaseComposter(RTDB& db);
};

#endif // COMMUNICATOR_H