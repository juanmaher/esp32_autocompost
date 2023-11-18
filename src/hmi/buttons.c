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

ESP_EVENT_DEFINE_BASE(BUTTON_EVENT);

static const char *TAG = "AC_Buttons";

static button_t crusher_btn, mixer_btn, fan_btn;

static const char *states[] = {
    [BUTTON_PRESSED]      = "pressed",
    [BUTTON_RELEASED]     = "released",
    [BUTTON_CLICKED]      = "clicked",
    [BUTTON_PRESSED_LONG] = "pressed long",
};

static void on_crusher_button(button_t *btn, button_state_t state);
static void on_mixer_button(button_t *btn, button_state_t state);
static void on_fan_button(button_t *btn, button_state_t state);

void Buttons_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Button connected between GPIO and +3.3V
    // pressed logic level 1, no autorepeat
    crusher_btn.gpio = CRUSHER_BUTTON_GPIO;
    crusher_btn.pressed_level = 1;
    crusher_btn.internal_pull = true;
    crusher_btn.autorepeat = false;
    crusher_btn.callback = on_crusher_button;

    // Button connected between GPIO and +3.3V
    // pressed logic level 1, no autorepeat
    mixer_btn.gpio = MIXER_BUTTON_GPIO;
    mixer_btn.pressed_level = 1;
    mixer_btn.internal_pull = true;
    mixer_btn.autorepeat = false;
    mixer_btn.callback = on_mixer_button;

    // Button connected between GPIO and +3.3V
    // pressed logic level 1, no autorepeat
    fan_btn.gpio = FAN_BUTTON_GPIO;
    fan_btn.pressed_level = 1;
    fan_btn.internal_pull = true;
    fan_btn.autorepeat = false;
    fan_btn.callback = on_fan_button;

    ESP_ERROR_CHECK(button_init(&crusher_btn));
    ESP_ERROR_CHECK(button_init(&mixer_btn));
    ESP_ERROR_CHECK(button_init(&fan_btn));
}

static void on_crusher_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Crusher button %s", states[state]);

    if (state == BUTTON_PRESSED) {
        esp_event_post(CRUSHER_EVENT, BUTTON_EVENT_CRUSHER_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}

static void on_mixer_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Mixer button %s", states[state]);

    if (state == BUTTON_PRESSED) {
        esp_event_post(MIXER_EVENT, BUTTON_EVENT_MIXER_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}

static void on_fan_button(button_t *btn, button_state_t state) {
    ESP_LOGI(TAG, "Fan button %s", states[state]);

    if (state == BUTTON_PRESSED) {
        esp_event_post(FAN_EVENT, BUTTON_EVENT_FAN_MANUAL_ON, NULL, 0, portMAX_DELAY);
    }
}