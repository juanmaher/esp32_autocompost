/**
 * @file buttons.c
 * @brief Module for handling button events.
 *
 * This module includes functions to initialize and manage button events for the crusher, mixer, and fan buttons.
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "common/events.h"
#include "common/gpios.h"
#include "drivers/button.h"

#define DEBUG false

// Event base for button events
ESP_EVENT_DEFINE_BASE(BUTTON_EVENT);

static const char *TAG = "AC_Buttons";

// Button structures for crusher, mixer, and fan buttons
static button_t crusher_btn, mixer_btn, fan_btn;

// String representations of button states
static const char *states[] = {
    [BUTTON_PRESSED]      = "pressed",
    [BUTTON_RELEASED]     = "released",
    [BUTTON_CLICKED]      = "clicked",
    [BUTTON_PRESSED_LONG] = "pressed long",
};

// Forward declarations of button callback functions
static void on_crusher_button(button_t *btn, button_state_t state);
static void on_mixer_button(button_t *btn, button_state_t state);
static void on_fan_button(button_t *btn, button_state_t state);

/**
 * @brief Function to start button module.
 */
void Buttons_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Configure crusher button
    crusher_btn.gpio = CRUSHER_BUTTON_GPIO;
    crusher_btn.pressed_level = 1;
    crusher_btn.internal_pull = true;
    crusher_btn.autorepeat = false;
    crusher_btn.callback = on_crusher_button;

    // Configure mixer button
    mixer_btn.gpio = MIXER_BUTTON_GPIO;
    mixer_btn.pressed_level = 1;
    mixer_btn.internal_pull = true;
    mixer_btn.autorepeat = false;
    mixer_btn.callback = on_mixer_button;

    // Configure fan button
    fan_btn.gpio = FAN_BUTTON_GPIO;
    fan_btn.pressed_level = 1;
    fan_btn.internal_pull = true;
    fan_btn.autorepeat = false;
    fan_btn.callback = on_fan_button;

    // Initialize buttons
    ESP_ERROR_CHECK(button_init(&crusher_btn));
    ESP_ERROR_CHECK(button_init(&mixer_btn));
    ESP_ERROR_CHECK(button_init(&fan_btn));
}

/**
 * @brief Callback function for the crusher button.
 *
 * @param btn   Pointer to the button structure.
 * @param state Button state.
 */
static void on_crusher_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Crusher button %s", states[state]);

    // Post event when the crusher button is pressed
    if (state == BUTTON_PRESSED) {
        esp_event_post(CRUSHER_EVENT, BUTTON_EVENT_CRUSHER_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}

/**
 * @brief Callback function for the mixer button.
 *
 * @param btn   Pointer to the button structure.
 * @param state Button state.
 */
static void on_mixer_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Mixer button %s", states[state]);

    // Post event when the mixer button is pressed
    if (state == BUTTON_PRESSED) {
        esp_event_post(MIXER_EVENT, BUTTON_EVENT_MIXER_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}

/**
 * @brief Callback function for the fan button.
 *
 * @param btn   Pointer to the button structure.
 * @param state Button state.
 */
static void on_fan_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Fan button %s", states[state]);

    // Post event when the fan button is pressed
    if (state == BUTTON_PRESSED) {
        esp_event_post(FAN_EVENT, BUTTON_EVENT_FAN_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}
