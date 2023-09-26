#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

static const int CONNECTION_STATE_BIT = BIT0;
static const int MIXER_STATE_BIT = BIT1;
static const int CRUSHER_STATE_BIT = BIT2;
static const int FAN_STATE_BIT = BIT3;

void Communicator_Start();

#endif // COMMUNICATOR_H