/**
 * @file communicator.c
 * @brief Implementation of the communicator module for handling communication with Firebase.
 */
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

#define RUTINE_COMMUNICATOR_TIMER_MS      6 * 60 * 60 * 1000 /* 21600000 ms */

ESP_EVENT_DEFINE_BASE(COMMUNICATOR_EVENT);

static const char *TAG = "AC_Communicator";
static const char firebase_path[] = "/composters/000002";

static bool wifi_connected = false;
static bool firebase_active_session = false;
static bool mixer_current_state;
static bool crusher_current_state;
static bool fan_current_state;

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
static void configure_firebase_connection();

/**
 * @brief Start the Communicator module.
 *
 * Initialize event handlers, tasks, and timers for communication.
 */
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

    mixer_current_state = ComposterParameters_GetMixerState(&composterParameters);
    crusher_current_state = ComposterParameters_GetCrusherState(&composterParameters);
    fan_current_state = ComposterParameters_GetFanState(&composterParameters);
}

/**
 * @brief Get the current data of the composter from Firebase.
 *
 * @return cJSON object containing the composter data.
 */
static cJSON * get_firebase_composter_data() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    cJSON *data_json = db->getData(db, firebase_path);
    if (DEBUG) ESP_LOGI(TAG, "%s: %s", __func__, cJSON_PrintUnformatted(data_json));

    if (data_json == NULL) {
        data_json = create_firebase_composter();
    }

    return data_json;
}

/**
 * @brief Create default composter data on Firebase.
 *
 * @return cJSON object representing the default composter data.
 */
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

/**
 * @brief Update the values of sensors parameters in Firebase.
 *
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
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
        cJSON_SetNumberValue(temperatureField, (int) temperature);
        cJSON_SetNumberValue(humidityField, (int) humidity);
        cJSON_SetNumberValue(completeField, (int) complete);
    } else {
        return ESP_FAIL;
    }

    if (DEBUG) ESP_LOGI(TAG, "%s: %s", __func__, cJSON_PrintUnformatted(data_json));

    db->putDataJson(db, firebase_path, data_json);

    cJSON_Delete(data_json);

    return ESP_OK;
}

/**
 * @brief Configure the Firebase connection with user credentials.
 */
static void configure_firebase_connection() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    user_data_t account = {USER_EMAIL, USER_PASSWORD};
    db = RTDB_Create(API_KEY, account, DATABASE_URL);
    firebase_active_session = true;
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
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

/**
 * @brief Timer callback function for periodic communication with Firebase.
 */
static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    if (wifi_connected && firebase_active_session) {
        update_sensors_parameters_values();
    }
}

/**
 * @brief Task to handle the periodic connection status check.
 *
 * This task monitors the Wi-Fi connection state and initiates or terminates the communication
 * timer accordingly. It also establishes or closes the connection to Firebase based on the Wi-Fi state.
 *
 * @param param Pointer to additional data (not used).
 */
static void connection_task(void* param) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Variables to track event bits and previous event bits
    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);

    // Flag to identify the first connection attempt
    bool first_connection = true;

    while (true) {
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        if (DEBUG) ESP_LOGD(TAG, "Connection Task Stack High Water Mark: %u bytes", stackHighWaterMark * sizeof(StackType_t));

        // Retrieve the current event bits
        uxBits = xEventGroupGetBits(s_communication_event_group);

        // Check if there is a change in event bits
        if (uxBits != prevBits) {
            // Check Wi-Fi connection state
            if ((uxBits & CONNECTION_STATE_BIT) != (prevBits & CONNECTION_STATE_BIT)) {
                ESP_LOGI(TAG, "Wi-Fi connection state has changed");

                // Wi-Fi is connected
                if (uxBits & CONNECTION_STATE_BIT) {
                    ESP_LOGI(TAG, "Wi-Fi connection active");
                    wifi_connected = true;

                    // Create the communicator timer on the first connection
                    if (first_connection) {
                        first_connection = false;
                        communicatorTimer = xTimerCreate("CommunicatorTimer", pdMS_TO_TICKS(RUTINE_COMMUNICATOR_TIMER_MS), pdTRUE, NULL, timer_callback_function);
                    }
                    configure_firebase_connection();
                    xTimerStart(communicatorTimer, portMAX_DELAY);
                }
                // Wi-Fi is disconnected
                else {
                    ESP_LOGI(TAG, "Wi-Fi connection inactive");
                    wifi_connected = false;
                    firebase_active_session = false;
                    xTimerStop(communicatorTimer, portMAX_DELAY);
                }
            }

            // Update previous event bits
            prevBits = uxBits;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}


/**
 * @brief Task to read changes from Firebase and trigger corresponding events.
 *
 * This task continuously monitors changes in the Firebase database related to mixer, crusher, and fan states.
 * When changes are detected, it generates corresponding events to handle manual control actions.
 *
 * @param param Pointer to additional data (not used).
 */
static void reading_changes_task(void* param) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    while (true) {
        // Check if Wi-Fi is connected and there is an active Firebase session
        if (wifi_connected && firebase_active_session) {
            UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (DEBUG) ESP_LOGD(TAG, "Reading Task Stack High Water Mark: %u bytes", stackHighWaterMark * sizeof(StackType_t));

            // Retrieve the Firebase data
            cJSON* data_json = get_firebase_composter_data();
            cJSON* mixerField = cJSON_GetObjectItem(data_json, "mezcladora");
            cJSON* crusherField = cJSON_GetObjectItem(data_json, "trituradora");
            cJSON* fanField = cJSON_GetObjectItem(data_json, "fan");

            // Check if the necessary fields exist and are of boolean type
            if (cJSON_IsBool(mixerField) && cJSON_IsBool(crusherField) && cJSON_IsBool(fanField)) {
                // Extract the boolean values
                bool mixer = cJSON_IsTrue(mixerField);
                bool crusher = cJSON_IsTrue(crusherField);
                bool fan = cJSON_IsTrue(fanField);

                // Check for changes in the mixer state
                if (mixer != mixer_current_state) {
                    // If mixer is turned on, generate a manual mixer start event
                    if (mixer) {
                        ESP_LOGI(TAG, "Manual mixer start detected");
                        esp_event_post(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_MIXER_MANUAL_ON, NULL, 0, portMAX_DELAY);
                    }
                }

                // Check for changes in the crusher state
                if (crusher != crusher_current_state) {
                    // If crusher is turned on, generate a manual crusher start event
                    if (crusher) {
                        ESP_LOGI(TAG, "Manual crusher start detected");
                        esp_event_post(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_CRUSHER_MANUAL_ON, NULL, 0, portMAX_DELAY);
                    }
                }

                // Check for changes in the fan state
                if (fan != fan_current_state) {
                    // If fan is turned on, generate a manual fan start event
                    if (fan) {
                        ESP_LOGI(TAG, "Manual fan start detected");
                        esp_event_post(COMMUNICATOR_EVENT, COMMUNICATOR_EVENT_FAN_MANUAL_ON, NULL, 0, portMAX_DELAY);
                    }
                }
            }
            cJSON_Delete(data_json);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

/**
 * @brief Task to write changes to Firebase based on local events.
 *
 * This task monitors local events related to mixer, crusher, and fan states and updates the corresponding data
 * in the Firebase database when changes are detected. It utilizes patch requests to minimize data transmission.
 *
 * @param param Pointer to additional data (not used).
 */
static void writing_changes_task(void* param) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Variables to track event bits and previous event bits
    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);

    while (true) {
        // Check if Wi-Fi is connected and there is an active Firebase session
        if (wifi_connected && firebase_active_session) {
            UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (DEBUG) ESP_LOGD(TAG, "Writing Task Stack High Water Mark: %u bytes", stackHighWaterMark * sizeof(StackType_t));

            // Retrieve the current event bits
            uxBits = xEventGroupGetBits(s_communication_event_group);
            if (uxBits != prevBits) {
                // Retrieve the current Firebase data
                cJSON* data_json = get_firebase_composter_data();

                // Check for changes in the mixer state
                if ((uxBits & MIXER_STATE_BIT) != (prevBits & MIXER_STATE_BIT)) {
                    ESP_LOGI(TAG, "Mixer state change detected");
                    cJSON* mixerField = cJSON_GetObjectItem(data_json, "mezcladora");
                    if (cJSON_IsBool(mixerField)) {
                        cJSON_ReplaceItemInObject(data_json, "mezcladora", cJSON_CreateBool(uxBits & MIXER_STATE_BIT ? true : false));
                    }
                    // Perform a patch request to update the Firebase data
                    db->patchDataJson(db, firebase_path, data_json);
                    mixer_current_state = uxBits & MIXER_STATE_BIT ? true : false;
                }

                // Check for changes in the crusher state
                if ((uxBits & CRUSHER_STATE_BIT) != (prevBits & CRUSHER_STATE_BIT)) {
                    ESP_LOGI(TAG, "Crusher state change detected");
                    cJSON* crusherField = cJSON_GetObjectItem(data_json, "trituradora");
                    if (cJSON_IsBool(crusherField)) {
                        cJSON_ReplaceItemInObject(data_json, "trituradora", cJSON_CreateBool(uxBits & CRUSHER_STATE_BIT ? true : false));
                    }
                    // Perform a patch request to update the Firebase data
                    db->patchDataJson(db, firebase_path, data_json);
                    crusher_current_state = uxBits & CRUSHER_STATE_BIT ? true : false;
                }

                // Check for changes in the fan state
                if ((uxBits & FAN_STATE_BIT) != (prevBits & FAN_STATE_BIT)) {
                    ESP_LOGI(TAG, "Fan state change detected");
                    cJSON* fanField = cJSON_GetObjectItem(data_json, "fan");
                    if (cJSON_IsBool(fanField)) {
                        cJSON_ReplaceItemInObject(data_json, "fan", cJSON_CreateBool(uxBits & FAN_STATE_BIT ? true : false));
                    }
                    // Perform a patch request to update the Firebase data
                    db->patchDataJson(db, firebase_path, data_json);
                    fan_current_state = uxBits & FAN_STATE_BIT ? true : false;
                }

                cJSON_Delete(data_json);
                prevBits = uxBits;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}
