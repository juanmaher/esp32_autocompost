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

static const char *TAG = "AC_Buttons";

static button_t crusher_btn, mixer_btn;

static const char *states[] = {
    [BUTTON_PRESSED]      = "pressed",
    [BUTTON_RELEASED]     = "released",
    [BUTTON_CLICKED]      = "clicked",
    [BUTTON_PRESSED_LONG] = "pressed long",
};

static void on_crusher_button(button_t *btn, button_state_t state);
static void on_mixer_button(button_t *btn, button_state_t state);

void Buttons_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // First button connected between GPIO and GND
    // pressed logic level 0, no autorepeat
    crusher_btn.gpio = CRUSHER_BUTTON_GPIO;
    crusher_btn.pressed_level = 0;
    crusher_btn.internal_pull = true;
    crusher_btn.autorepeat = false;
    crusher_btn.callback = on_crusher_button;

    // Second button connected between GPIO and +3.3V
    // pressed logic level 1, autorepeat enabled
    mixer_btn.gpio = MIXER_BUTTON_GPIO;
    mixer_btn.pressed_level = 1;
    mixer_btn.internal_pull = true;
    mixer_btn.autorepeat = true;
    mixer_btn.callback = on_mixer_button;

    ESP_ERROR_CHECK(button_init(&crusher_btn));
    ESP_ERROR_CHECK(button_init(&mixer_btn));
}

static void on_crusher_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Crusher button %s", states[state]);

    if (state == BUTTON_PRESSED) {
        esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}

static void on_mixer_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Mixer button %s", states[state]);

    if (state == BUTTON_PRESSED) {
        esp_event_post(MIXER_EVENT, MIXER_EVENT_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}