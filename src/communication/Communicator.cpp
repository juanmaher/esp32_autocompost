#include "esp_wifi.h"

#include "communication/Communicator.hpp"

ESP_EVENT_DEFINE_BASE(CRUSHER_EVENT);
ESP_EVENT_DEFINE_BASE(MIXER_EVENT);

static TimerHandle_t communicatorTimer = NULL;
static EventGroupHandle_t s_communication_event_group;
static const char *TAG = "Communicator";
static const bool DEBUG = true;
std::string firebase_path = "/composters/" + std::string(COMPOSTER_ID);
bool wifi_connected = false;

static void timer_callback_function(TimerHandle_t xTimer) {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    Communicator* communicatorInstance = static_cast<Communicator*>(pvTimerGetTimerID(xTimer));
    if (communicatorInstance != nullptr) {
        communicatorInstance->updateSensorsParametersValues(xTimer, communicatorInstance->db, communicatorInstance->composterParameters);
    }
}

static void communicator_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %lu", event_base, event_id);

    Communicator* communicatorInstance = static_cast<Communicator*>(arg);

    if (strcmp(event_base, WIFI_EVENT) == 0) {
        if (event_id == WIFI_EVENT_CONNECTION_ON) {
            communicatorInstance->configureFirebaseConnection();
            wifi_connected = true;
        } else if (event_id == WIFI_EVENT_CONNECTION_OFF) {
            wifi_connected = false;
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

void writingChangesTask(void* param) {
    EventBits_t uxBits;
    EventBits_t prevBits = xEventGroupGetBits(s_communication_event_group);
    Communicator* communicatorInstance = static_cast<Communicator*>(param);

    while (wifi_connected) {
        uxBits = xEventGroupWaitBits(s_communication_event_group, MIXER_STATE_BIT | CRUSHER_STATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
        Json::Value data = Communicator::getFirebaseComposterData(communicatorInstance->db);

        // Detectar cambios en el bit de estado de la mezcladora
        if ((uxBits & MIXER_STATE_BIT) != (prevBits & MIXER_STATE_BIT)) {
            if (DEBUG) ESP_LOGD(TAG, "Se detecto cambio estado de la mezcladora");
            communicatorInstance->composterParameters.getMixerState();
            if (uxBits & MIXER_STATE_BIT) {
                communicatorInstance->composterParameters.setMixerState(true);
            } else {
                communicatorInstance->composterParameters.setMixerState(false);
            }
            data["mixer"] = communicatorInstance->composterParameters.getMixerState();
            communicatorInstance->db.patchData(firebase_path.c_str(), data);
        }

        // Detectar cambios en el bit de estado del triturador
        if ((uxBits & CRUSHER_STATE_BIT) != (prevBits & CRUSHER_STATE_BIT)) {
            if (DEBUG) ESP_LOGD(TAG, "Se detecto cambio estado de la trituradora");
            communicatorInstance->composterParameters.getCrusherState();
            if (uxBits & CRUSHER_STATE_BIT) {
                communicatorInstance->composterParameters.setCrusherState(true);
            } else {
                communicatorInstance->composterParameters.setCrusherState(false);
            }
            data["crusher"] = communicatorInstance->composterParameters.getCrusherState();
            communicatorInstance->db.patchData(firebase_path.c_str(), data);
        }

        // Guardar los bits actuales para la próxima iteración
        prevBits = uxBits;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

void readingChangesTask(void* param) {
    Communicator* communicatorInstance = static_cast<Communicator*>(param);

    while (wifi_connected) {
        Json::Value data = Communicator::getFirebaseComposterData(communicatorInstance->db);
        
        if (data["mixer"].asBool() != communicatorInstance->composterParameters.getMixerState()) {
            if (true == data["mixer"].asBool()) {
                if (DEBUG) ESP_LOGD(TAG, "Inicio manual de mezcladora");
                esp_event_post(MIXER_EVENT, static_cast<int>(MIXER_EVENT_MANUAL_ON), nullptr, 0, portMAX_DELAY);
            } else if (false == data["mixer"].asBool()) {
                if (DEBUG) ESP_LOGD(TAG, "Apagado manual de mezcladora");
                esp_event_post(MIXER_EVENT, static_cast<int>(MIXER_EVENT_MANUAL_OFF), nullptr, 0, portMAX_DELAY);
            }
        }

        if (data["crusher"].asBool() != communicatorInstance->composterParameters.getCrusherState()) {
            if (true == data["crusher"].asBool()) {
                if (DEBUG) ESP_LOGD(TAG, "Inicio manual de trituradora");
                esp_event_post(CRUSHER_EVENT, static_cast<int>(CRUSHER_EVENT_MANUAL_ON), nullptr, 0, portMAX_DELAY);
            } else if (false == data["crusher"].asBool()) {
                if (DEBUG) ESP_LOGD(TAG, "Apagado manual de trituradora");
                esp_event_post(CRUSHER_EVENT, static_cast<int>(CRUSHER_EVENT_MANUAL_OFF), nullptr, 0, portMAX_DELAY);
            }
        }

        if (data["fan"].asBool() != communicatorInstance->composterParameters.getFanState()) {
            if (true == data["fan"].asBool()) {
                if (DEBUG) ESP_LOGD(TAG, "Apagado manual de ventilador");
                // Evento de inicio de ventilador
            } else if (false == data["fan"].asBool()) {
                if (DEBUG) ESP_LOGD(TAG, "Apagado manual de ventilador");
                // Evento de fin de ventilador
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

Json::Value Communicator::getFirebaseComposterData(RTDB& db) {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    Json::Value data = db.getData(firebase_path.c_str());

    if (data.isNull()) {
        data = Communicator::createFirebaseComposter(db);
    }

    return data;
}

Json::Value Communicator::createFirebaseComposter(RTDB& db) {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    std::string json_str = R"({"complete": 0, "days": 0, "humidity": 0, "temperature": 0, "mixer": false, "crusher": false, "fan": false})";
    db.putData(firebase_path.c_str(), json_str.c_str()); 
    return db.getData(firebase_path.c_str());
}

void Communicator::updateSensorsParametersValues(TimerHandle_t xTimer, RTDB& db, ComposterParameters& composterParameters) {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    Json::Value data = Communicator::getFirebaseComposterData(db);

    // Acceder a los valores de composterParameters
    double temperature = composterParameters.getTemperature();
    double humidity = composterParameters.getHumidity();
    double complete = composterParameters.getComplete();

    // Actualizar los valores en el objeto 'data'
    data["temperature"] = temperature;
    data["humidity"] = humidity;
    data["complete"] = complete;

    db.patchData(firebase_path.c_str(), data);
}

Communicator::Communicator() {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    s_communication_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(MIXER_EVENT, ESP_EVENT_ANY_ID, &communicator_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(CRUSHER_EVENT, ESP_EVENT_ANY_ID, &communicator_event_handler, NULL));

    communicatorTimer = xTimerCreate("CommunicatorTimer", pdMS_TO_TICKS(6 * 60 * 60 * 1000), pdTRUE, NULL, timer_callback_function);
}

Communicator::~Communicator() {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);
    wifi_connected = false;
}

void Communicator::configureFirebaseConnection() {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    user_account_t account = {USER_EMAIL, USER_PASSWORD};
    FirebaseApp app = FirebaseApp(API_KEY);
    app.loginUserAccount(account);
    db.initialize(&app, DATABASE_URL);
}

void Communicator::start() {
    if (DEBUG) ESP_LOGD(TAG, "on %s", __func__);

    xTaskCreate(readingChangesTask, "readingChangesTask", 1000, NULL, 1, NULL);
    xTaskCreate(writingChangesTask, "writingChangesTask", 1000, NULL, 1, NULL);
    xTimerStart(communicatorTimer, portMAX_DELAY);
}