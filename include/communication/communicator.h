#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <string.h>

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

static const int CONNECTION_STATE_BIT = BIT0;
static const int MIXER_STATE_BIT = BIT1;
static const int CRUSHER_STATE_BIT = BIT2;
static const int FAN_STATE_BIT = BIT3;

void Communicator_start();

#endif // COMMUNICATOR_H