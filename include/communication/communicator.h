#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

/**
 * @file communicator.h
 * @brief Declarations for the communicator module.
 */

static const int CONNECTION_STATE_BIT = BIT0;
static const int MIXER_STATE_BIT = BIT1;
static const int CRUSHER_STATE_BIT = BIT2;
static const int FAN_STATE_BIT = BIT3;

/**
 * @brief Initializes the communicator module.
 */
void Communicator_Start();

#endif // COMMUNICATOR_H