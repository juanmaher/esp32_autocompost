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

#include "common/events.h"
#include "common/composter_parameters.h"
#include "config/firebase_config.h"
#include "communication/communicator.h"

#define DEBUG false

static const char *TAG = "AC_Communicator";
static const char firebase_path[] = "/composters/000002";

static bool wifi_connected = false;
static bool firebase_active_session = false;

extern ComposterParameters composterParameters;

static TimerHandle_t communicatorTimer = NULL;
static EventGroupHandle_t s_communication_event_group;

static RTDB_t * db;

static void timer_callback_function(TimerHandle_t xTimer);
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void writing_changes_task(void* param);
static void reading_changes_task(void* param);
static void connection_task(void* param);
static cJSON * get_firebase_composter_data();
static cJSON * create_firebase_composter();
static esp_err_t update_sensors_parameters_values();

void Communicator_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    s_communication_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(FAN_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT_INTERNAL, ESP_EVENT_ANY_ID, &event_handler, NULL));

    xTaskCreate(connection_task, "connection_task", 8192, NULL, 3, NULL); 
    xTaskCreate(reading_changes_task, "reading_changes_task", 8192, NULL, 3, NULL);
    xTaskCreate(writing_changes_task, "writing_changes_task", 8192, NULL, 3, NULL);
}

static cJSON * get_firebase_composter_data() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    cJSON *data_json = db->getData(db, firebase_path);
    if (DEBUG) ESP_LOGI(TAG, "%s: %s", __func__, cJSON_PrintUnformatted(data_json));

    if (data_json == NULL) {
        data_json = create_firebase_composter();
    }

    return data_json;
}

static cJSON * create_firebase_composter() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    cJSON *data_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(data_json, "complete", 0);
    cJSON_AddNumberToObject(data_json, "days", 0);
    cJSON_AddNumberToObject(data_json, "humidity", 0);
    cJSON_AddNumberToObject(data_json, "temperature", 0);
    cJSON_AddBoolToObject(data_json, "mixer", false);
    cJSON_AddBoolToObject(data_json, "crusher", false);
    cJSON_AddBoolToObject(data_json, "fan", false);

    if (db->putDataJson(db, firebase_path, data_json)) {
        return NULL;
    }

    return data_json;
}

static esp_err_t update_sensors_parameters_values() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    double temperature = ComposterParameters_GetTemperature(&composterParameters);
    double humidity = ComposterParameters_GetHumidity(&composterParameters);
    double complete = ComposterParameters_GetComplete(&composterParameters);

    if (DEBUG) ESP_LOGI(TAG, "temperature: %f", temperature);
    if (DEBUG) ESP_LOGI(TAG, "humidity: %f", humidity);
    if (DEBUG) ESP_LOGI(TAG, "complete: %f", complete);

    cJSON *data_json = get_firebase_composter_data();

    cJSON *temperatureField = cJSON_GetObjectItem(data_json, "temperature");
    cJSON *humidityField = cJSON_GetObjectItem(data_json, "humidity");
    cJSON *completeField = cJSON_GetObjectItem(data_json, "complete");

    if (DEBUG) ESP_LOGI(TAG, "temperatureField: %f", cJSON_GetNumberValue(temperatureField));
    if (DEBUG) ESP_LOGI(TAG, "humidityField: %f", cJSON_GetNumberValue(humidityField));
    if (DEBUG) ESP_LOGI(TAG, "completeField: %f", cJSON_GetNumberValue(completeField));

    if (cJSON_IsNumber(temperatureField) && cJSON_IsNumber(humidityField) && cJSON_IsNumber(completeField)) {
        cJSON_SetNumberValue(temperatureField, temperature);
        cJSON_SetNumberValue(humidityField, humidity);
        cJSON_SetNumberValue(completeField, complete);
    } else {
        return ESP_FAIL;
    }

    if (DEBUG) ESP_LOGI(TAG, "%s: %s", __func__, cJSON_PrintUnformatted(data_json));

    db->putDataJson(db, firebase_path, data_json);

    cJSON_Delete(data_json);

    return ESP_OK;
}

static void Communicator_configureFirebaseConnection() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    user_data_t account = {USER_EMAIL, USER_PASSWORD};
    db = RTDB_Create(API_KEY, account, DATABASE_URL);
    firebase_active_session = true;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
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
    } else if (strcmp(event_base, FAN_EVENT) == 0) {
        if (event_id == FAN_EVENT_ON) {
            xEventGroupSetBits(s_communication_event_group, FAN_STATE_BIT);
        } else if (event_id == FAN_EVENT_OFF) {
            xEventGroupClearBits(s_communication_event_group, FAN_STATE_BIT);
        }
    }
}

static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (wifi_connected && firebase_active_session) {
        update_sensors_parameters_values();
    }
}

static void connection_task(void* param) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);

    bool first_connection = true;

    while (true) {

        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        if (DEBUG) ESP_LOGI(TAG, "Connection Task Stack High Water Mark: %u bytes", stackHighWaterMark * sizeof(StackType_t));

        uxBits = xEventGroupWaitBits(s_communication_event_group, CONNECTION_STATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

        if ((uxBits & CONNECTION_STATE_BIT) != (prevBits & CONNECTION_STATE_BIT)) {
            ESP_LOGI(TAG, "Wi-Fi connection state has changed");
            if (uxBits & CONNECTION_STATE_BIT) {
                ESP_LOGI(TAG, "Wi-Fi connection active");
                wifi_connected = true;
                if (first_connection) {
                    first_connection = false;
                    Communicator_configureFirebaseConnection();
                    /* 6 hours = 6 * 60 * 60 * 1000 = 21600000 ms*/
                    communicatorTimer = xTimerCreate("CommunicatorTimer", pdMS_TO_TICKS(21600000), pdTRUE, NULL, timer_callback_function);
                }
                xTimerStart(communicatorTimer, portMAX_DELAY);
            } else {
                ESP_LOGI(TAG, "Wi-Fi connection inactive");
                wifi_connected = false;
                xTimerStop(communicatorTimer, portMAX_DELAY);
            }
        }

        prevBits = uxBits;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

static void reading_changes_task(void* param) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    while (true) {

        if (firebase_active_session) {

            UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (DEBUG) ESP_LOGI(TAG, "Reading Task Stack High Water Mark: %u bytes", stackHighWaterMark * sizeof(StackType_t));

            cJSON* data_json = get_firebase_composter_data();
            cJSON* mixerField = cJSON_GetObjectItem(data_json, "mixer");
            cJSON* crusherField = cJSON_GetObjectItem(data_json, "crusher");
            cJSON* fanField = cJSON_GetObjectItem(data_json, "fan");

            if (cJSON_IsBool(mixerField) && cJSON_IsBool(crusherField) && cJSON_IsBool(fanField)) {
                bool mixer = cJSON_IsTrue(mixerField);
                bool crusher = cJSON_IsTrue(crusherField);
                bool fan = cJSON_IsTrue(fanField);

                if (mixer != ComposterParameters_GetMixerState(&composterParameters)) {
                    if (mixer) {
                        ESP_LOGI(TAG, "Inicio manual de mezcladora");
                        esp_event_post(MIXER_EVENT, MIXER_EVENT_MANUAL_ON, NULL, 0, portMAX_DELAY);
                    } else {
                        ESP_LOGI(TAG, "Apagado manual de mezcladora");
                        esp_event_post(MIXER_EVENT, MIXER_EVENT_MANUAL_OFF, NULL, 0, portMAX_DELAY);
                    }
                }

                if (crusher != ComposterParameters_GetCrusherState(&composterParameters)) {
                    if (crusher) {
                        ESP_LOGI(TAG, "Inicio manual de trituradora");
                        esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_MANUAL_ON, NULL, 0, portMAX_DELAY);
                    } else {
                        ESP_LOGI(TAG, "Apagado manual de trituradora");
                        esp_event_post(CRUSHER_EVENT, CRUSHER_EVENT_MANUAL_OFF, NULL, 0, portMAX_DELAY);
                    }
                }

                if (fan != ComposterParameters_GetFanState(&composterParameters)) {
                    if (fan) {
                        ESP_LOGI(TAG, "Apagado manual de ventilador");
                        esp_event_post(FAN_EVENT, FAN_EVENT_MANUAL_ON, NULL, 0, portMAX_DELAY);
                    } else {
                        ESP_LOGI(TAG, "Apagado manual de ventilador");
                        esp_event_post(FAN_EVENT, FAN_EVENT_MANUAL_OFF, NULL, 0, portMAX_DELAY);
                    }
                }
            }
            cJSON_Delete(data_json);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    vTaskDelete(NULL);
}

static void writing_changes_task(void* param) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);

    while (true) {

        if (firebase_active_session) {

            UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (DEBUG) ESP_LOGI(TAG, "Writing Task Stack High Water Mark: %u bytes", stackHighWaterMark * sizeof(StackType_t));

            uxBits = xEventGroupWaitBits(s_communication_event_group, MIXER_STATE_BIT | CRUSHER_STATE_BIT | FAN_STATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
            cJSON* data_json = get_firebase_composter_data();

            if ((uxBits & MIXER_STATE_BIT) != (prevBits & MIXER_STATE_BIT)) {
                ESP_LOGI(TAG, "Se detecto cambio estado de la mezcladora");
                cJSON* mixerField = cJSON_GetObjectItem(data_json, "mixer");
                if (cJSON_IsBool(mixerField)) {
                    cJSON_ReplaceItemInObject(data_json, "mixer", cJSON_CreateBool(ComposterParameters_GetMixerState(&composterParameters)));
                }
            }

            if ((uxBits & CRUSHER_STATE_BIT) != (prevBits & CRUSHER_STATE_BIT)) {
                ESP_LOGI(TAG, "Se detecto cambio estado de la trituradora");
                cJSON* crusherField = cJSON_GetObjectItem(data_json, "crusher");
                if (cJSON_IsBool(crusherField)) {
                    cJSON_ReplaceItemInObject(data_json, "crusher", cJSON_CreateBool(ComposterParameters_GetMixerState(&composterParameters)));
                }
            }

            if ((uxBits & FAN_STATE_BIT) != (prevBits & FAN_STATE_BIT)) {
                ESP_LOGI(TAG, "Se detecto cambio estado del ventilador");
                cJSON* fanField = cJSON_GetObjectItem(data_json, "fan");
                if (cJSON_IsBool(fanField)) {
                    cJSON_ReplaceItemInObject(data_json, "fan", cJSON_CreateBool(ComposterParameters_GetMixerState(&composterParameters)));
                }
            }

            db->patchDataJson(db, firebase_path, data_json);

            prevBits = uxBits;
            cJSON_Delete(data_json);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}
