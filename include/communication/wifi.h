#ifndef WIFI_H
#define WIFI_H

/**
 * @file wifi.h
 * @brief Declarations for the wifi module.
 */

static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;

/**
 * @brief Initializes the Wi-Fi module.
 */
void Wifi_Start();

#endif // WIFI_H
