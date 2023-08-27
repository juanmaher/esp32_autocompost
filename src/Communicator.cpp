#include "Communicator.hpp"
#include "Mixer.hpp"
#include "Crusher.hpp"

static EventGroupHandle_t s_communication_event_group;
static const char *TAG = "Communicator";

static void communicator_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {

    ESP_LOGI(TAG, "Event received: %s, %lu", event_base, event_id);

    if (strcmp(event_base, MIXER_EVENT) == 0) {
        if (event_id == MIXER_EVENT_ON) {
            xEventGroupSetBits(s_communication_event_group, MIXER_STATE_BIT);
        } else if (event_id == MIXER_EVENT_OFF) {
            xEventGroupClearBits(s_communication_event_group, MIXER_STATE_BIT);
        }
    } else if (strcmp(event_base, CRUSHER_EVENT) == 0) {
        if (event_id == CRUSHER_EVENT_ON) {
            xEventGroupSetBits(s_communication_event_group, CRUSHER_STATE_BIT);
        } else if (event_id == CRUSHER_EVENT_OFF) {
            xEventGroupClearBits(s_communication_event_group, CRUSHER_STATE_BIT);
        }
    } else {
        // Default case
    }
}

void writingChangesTask(void* param) {
    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);

    // Mientras este conectado a Wi-Fi
    while (true) {
        uxBits = xEventGroupWaitBits(s_communication_event_group, MIXER_STATE_BIT | CRUSHER_STATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

        // Detectar cambios en el bit de estado de la mezcladora
        if ((uxBits & MIXER_STATE_BIT) != (prevBits & MIXER_STATE_BIT)) {
            if (uxBits & MIXER_STATE_BIT) {
                // El bit cambió a 1 (encendido)
                // Realizar acciones cuando la mezcladora se enciende
            } else {
                // El bit cambió a 0 (apagado)
                // Realizar acciones cuando la mezcladora se apaga
            }
        }

        // Detectar cambios en el bit de estado del triturador
        if ((uxBits & CRUSHER_STATE_BIT) != (prevBits & CRUSHER_STATE_BIT)) {
            if (uxBits & CRUSHER_STATE_BIT) {
                // El bit cambió a 1 (encendido)
                // Realizar acciones cuando la mezcladora se enciende
            } else {
                // El bit cambió a 0 (apagado)
                // Realizar acciones cuando la mezcladora se apaga
            }
        }

        // Guardar los bits actuales para la próxima iteración
        prevBits = uxBits;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

void readingChangesTask(void* param) {

    // Mientras este conectado a Wi-Fi
    while (true) {
        
        // Chequear si cambio el estado de alguno de los botones
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

Communicator::Communicator() { 
    s_communication_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, ESP_EVENT_ANY_ID, &communicator_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, ESP_EVENT_ANY_ID, &communicator_event_handler, NULL));
    //ESP_ERROR_CHECK(esp_event_handler_register(TIME_EVENT, ESP_EVENT_ANY_ID, &communicator_event_handler, NULL));
}

void Communicator::start() {
    xTaskCreate(readingChangesTask, "readingChangesTask", 1000, NULL, 1, NULL);
    xTaskCreate(writingChangesTask, "writingChangesTask", 1000, NULL, 1, NULL);
}
