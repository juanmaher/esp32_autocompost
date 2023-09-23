#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "cJSON.h"
#include "rtdb_wrapper.h"

//#include "common/ComposterParameters.h"
#include "communication/communicator.h"
#include "common/events.h"
#include "config/firebase_config.h"

ESP_EVENT_DEFINE_BASE(CRUSHER_EVENT);
ESP_EVENT_DEFINE_BASE(MIXER_EVENT);
ESP_EVENT_DEFINE_BASE(WIFI_EVENT_INTERNAL);

static const char *TAG = "AC_Communicator";
static const char firebase_path[] = "/composters/000005";

static bool wifi_connected = false;
static bool firebase_active_session = false;

static TimerHandle_t communicatorTimer = NULL;
static EventGroupHandle_t s_communication_event_group;

static void timer_callback_function(TimerHandle_t xTimer);
static void Communicator_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
//static void writingChangesTask(void* param);
static void readingChangesTask(void* param);
static void connectionTask(void* param);
static void Communicator_getFirebaseComposterData();
static void Communicator_createFirebaseComposter();

static RTDB_t * db;
//static ComposterParameters_t composterParameters;

void Communicator_start() {
    ESP_LOGI(TAG, "on %s", __func__);

    s_communication_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, ESP_EVENT_ANY_ID, &Communicator_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, ESP_EVENT_ANY_ID, &Communicator_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT_INTERNAL, ESP_EVENT_ANY_ID, &Communicator_event_handler, NULL));

    xTaskCreate(connectionTask, "connectionTask", 4096, NULL, 3, NULL); 
    xTaskCreate(readingChangesTask, "readingChangesTask", 4096, NULL, 3, NULL);
    //xTaskCreate(writingChangesTask, "writingChangesTask", 4096, NULL, 3, NULL);
}

static void Communicator_getFirebaseComposterData() {
    ESP_LOGI(TAG, "on %s", __func__);

    char json_str[256];
    strcpy(json_str, db->getData(db, firebase_path));

    ESP_LOGI(TAG, "%s", json_str);

    if (strcmp(json_str, "null") == 0) {
        Communicator_createFirebaseComposter();
    }
}

static void Communicator_createFirebaseComposter() {
    ESP_LOGI(TAG, "on %s", __func__);

    char json_str[] = "{\"complete\": 0, \"days\": 0, \"humidity\": 0, \"temperature\": 0, \"mixer\": false, \"crusher\": false, \"fan\": false}";
    db->putData(db, firebase_path, json_str);
}

/*static void Communicator_updateSensorsParametersValues(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "on %s", __func__);

    JsonValue data;
    Communicator_getFirebaseComposterData(db, &data);

    // Acceder a los valores de composterParameters
    double temperature = ComposterParameters_getTemperature(composterParameters);
    double humidity = ComposterParameters_getHumidity(composterParameters);
    double complete = ComposterParameters_getComplete(composterParameters);

    // Actualizar los valores en el objeto 'data'
    snprintf(data.data, 256, "{\"temperature\": %.2lf, \"humidity\": %.2lf, \"complete\": %.2lf}", temperature, humidity, complete);

    RTDB_patchData(db, firebase_path, data.data);
}*/

static void Communicator_configureFirebaseConnection() {
    ESP_LOGI(TAG, "on %s", __func__);

    user_data_t account = {USER_EMAIL, USER_PASSWORD};
    db = RTDB_Create(API_KEY, account, DATABASE_URL);
    firebase_active_session = true;
}

static void Communicator_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %ld", event_base, event_id);

    if (strcmp(event_base, WIFI_EVENT_INTERNAL) == 0) {
        if (event_id == WIFI_EVENT_CONNECTION_ON) {
            xEventGroupSetBits(s_communication_event_group, CONNECTION_STATE_BIT);
        } else if (event_id == WIFI_EVENT_CONNECTION_OFF) {
            xEventGroupClearBits(s_communication_event_group, CONNECTION_STATE_BIT);
        }
    } else if (strcmp(event_base, MIXER_EVENT) == 0) {
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
    }
}

static void timer_callback_function(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "on %s", __func__);

    if (wifi_connected && firebase_active_session) {
        //Communicator_updateSensorsParametersValues(xTimer);
    }
}

static void connectionTask(void* param) {
    ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);

    bool first_connection = true;

    while (true) {
        uxBits = xEventGroupWaitBits(s_communication_event_group, CONNECTION_STATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

        if ((uxBits & CONNECTION_STATE_BIT) != (prevBits & CONNECTION_STATE_BIT)) {
            ESP_LOGI(TAG, "Wi-Fi connection state has changed");
            if (uxBits & CONNECTION_STATE_BIT) {
                ESP_LOGI(TAG, "Wi-Fi connection active");
                wifi_connected = true;
                if (first_connection) {
                    first_connection = false;
                    Communicator_configureFirebaseConnection();
                    communicatorTimer = xTimerCreate("CommunicatorTimer", pdMS_TO_TICKS(3000), pdTRUE, NULL, timer_callback_function);
                }
                xTimerStart(communicatorTimer, portMAX_DELAY);
            } else {
                ESP_LOGI(TAG, "Wi-Fi connection inactive");
                wifi_connected = false;
                xTimerStop(communicatorTimer, portMAX_DELAY);
            }
        }

        // Guardar los bits actuales para la próxima iteración
        prevBits = uxBits;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

static void readingChangesTask(void* param) {
    ESP_LOGI(TAG, "on %s", __func__);

    while (true) {
        if (firebase_active_session) {
            Communicator_getFirebaseComposterData();
            /*ESP_LOGI(TAG, "mixer: %d", data["trituradora"].asBool());
            ESP_LOGI(TAG, "crusher: %d", data["mezcladora"].asBool());
            ESP_LOGI(TAG, "fan: %d", data["fan"].asBool());

            if (data["mixer"].asBool() != communicatorInstance->composterParameters.getMixerState()) {
                if (true == data["mixer"].asBool()) {
                    ESP_LOGI(TAG, "Inicio manual de mezcladora");
                    esp_event_post(MIXER_EVENT, static_cast<int>(MIXER_EVENT_MANUAL_ON), nullptr, 0, portMAX_DELAY);
                } else if (false == data["mixer"].asBool()) {
                    ESP_LOGI(TAG, "Apagado manual de mezcladora");
                    esp_event_post(MIXER_EVENT, static_cast<int>(MIXER_EVENT_MANUAL_OFF), nullptr, 0, portMAX_DELAY);
                }
            }

            if (data["crusher"].asBool() != communicatorInstance->composterParameters.getCrusherState()) {
                if (true == data["crusher"].asBool()) {
                    ESP_LOGI(TAG, "Inicio manual de trituradora");
                    esp_event_post(CRUSHER_EVENT, static_cast<int>(CRUSHER_EVENT_MANUAL_ON), nullptr, 0, portMAX_DELAY);
                } else if (false == data["crusher"].asBool()) {
                    ESP_LOGI(TAG, "Apagado manual de trituradora");
                    esp_event_post(CRUSHER_EVENT, static_cast<int>(CRUSHER_EVENT_MANUAL_OFF), nullptr, 0, portMAX_DELAY);
                }
            }

            if (data["fan"].asBool() != communicatorInstance->composterParameters.getFanState()) {
                if (true == data["fan"].asBool()) {
                    ESP_LOGI(TAG, "Apagado manual de ventilador");
                    // Evento de inicio de ventilador
                } else if (false == data["fan"].asBool()) {
                    ESP_LOGI(TAG, "Apagado manual de ventilador");
                    // Evento de fin de ventilador
                }
            }*/
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

/*static void writingChangesTask(void* param) {
    ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);
    Communicator* communicatorInstance = (Communicator*)param;

    while (true) {
        if (communicator_state.firebase_active_session) {
            uxBits = xEventGroupWaitBits(s_communication_event_group, MIXER_STATE_BIT | CRUSHER_STATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
            JsonValue data;
            Communicator_getFirebaseComposterData(&communicatorInstance->db, &data);

            // Detectar cambios en el bit de estado de la mezcladora
            if ((uxBits & MIXER_STATE_BIT) != (prevBits & MIXER_STATE_BIT)) {
                ESP_LOGI(TAG, "Se detecto cambio estado de la mezcladora");
                ComposterParameters_getMixerState(&communicatorInstance->composterParameters);
                if (uxBits & MIXER_STATE_BIT) {
                    ComposterParameters_setMixerState(&communicatorInstance->composterParameters, true);
                } else {
                    ComposterParameters_setMixerState(&communicatorInstance->composterParameters, false);
                }
                snprintf(data.data, 256, "{\"mixer\": %s}", ComposterParameters_getMixerState(&communicatorInstance->composterParameters) ? "true" : "false");
                RTDB_patchData(&communicatorInstance->db, firebase_path, data.data);
            }

            // Detectar cambios en el bit de estado del triturador
            if ((uxBits & CRUSHER_STATE_BIT) != (prevBits & CRUSHER_STATE_BIT)) {
                ESP_LOGI(TAG, "Se detecto cambio estado de la trituradora");
                ComposterParameters_getCrusherState(&communicatorInstance->composterParameters);
                if (uxBits & CRUSHER_STATE_BIT) {
                    ComposterParameters_setCrusherState(&communicatorInstance->composterParameters, true);
                } else {
                    ComposterParameters_setCrusherState(&communicatorInstance->composterParameters, false);
                }
                snprintf(data.data, 256, "{\"crusher\": %s}", ComposterParameters_getCrusherState(&communicatorInstance->composterParameters) ? "true" : "false");
                RTDB_patchData(&communicatorInstance->db, firebase_path, data.data);
            }

            // Guardar los bits actuales para la próxima iteración
            prevBits = uxBits;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}*/
