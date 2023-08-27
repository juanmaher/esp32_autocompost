#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

static const int MIXER_STATE_BIT = BIT0;
static const int CRUSHER_STATE_BIT = BIT1;
static const int TIME_STATE_BIT = BIT2;

// Class to communicate with cloud server
class Communicator {
    public:
        Communicator();
        void start();

private:

};

#endif // COMMUNICATOR_H