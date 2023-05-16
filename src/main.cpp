
#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"


#include "jsoncpp/value.h"
#include "jsoncpp/json.h"

#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"

#include "firebase_config.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
using namespace ESPFirebase;
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

ESP_EVENT_DECLARE_BASE(FIREBASE_EVENT);

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "smartconfig_example";
bool isConnected = false;

void event_callback_function_firebase();
static void smartconfig_example_task(void * parm);

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void initialise_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    //esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &event_callback_function_firebase, NULL);

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

    // Crear una instancia del evento personalizado
    firebase_event_data_t firebase_event;
    firebase_event.data = 123;
    strcpy(firebase_event.message, "Evento de Firebase");

    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
            // Disparar el evento FIREBASE_EVENT con un identificador específico
            //esp_event_post(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &firebase_event, sizeof(firebase_event_data_t), portMAX_DELAY);
            event_callback_function_firebase();
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void event_callback_function_firebase() {
    ESP_LOGI(TAG, "YA ME CONECTE");
    // Config and Authentication
    user_account_t account = {USER_EMAIL, USER_PASSWORD};
    ESP_LOGI(TAG, "Linea 156");
    FirebaseApp app = FirebaseApp(API_KEY);
    ESP_LOGI(TAG, "Linea 158");
    app.loginUserAccount(account);
    ESP_LOGI(TAG, "Linea 160");
    RTDB db = RTDB(&app, DATABASE_URL);

    

    // R"()" allow us to write string as they are without escaping the characters with backslash

    // We can put a json str directly at /person1
    std::string json_str = R"({"name": "Madjid", "age": 20, "random_float": 8.56})";
    db.putData("/person1", json_str.c_str()); 
    // vTaskDelay(500/portTICK_PERIOD_MS);

    // We can parse the json_str and access the members and edit them
    Json::Value data;
    Json::Reader reader; 
    reader.parse(json_str, data);

    std::string madjid_name = data["name"].asString();  // convert value to primitives (read jsoncpp docs for more of these)

    // ESP_LOGI("MAIN", "name: %s", madjid_name.c_str());   

    data["name"] = "Tikou";

    // ESP_LOGI("MAIN", "edited name from %s to: %s", madjid_name.c_str(), data["name"].asString().c_str());   

    data["age"] = 22;
    data["random_float"] = 4.44;
    
    // put json object directly
    db.putData("/person2", data);   

    // vTaskDelay(500/portTICK_PERIOD_MS);
    // Construct a new json object manually
    Json::Value new_data; 
    new_data["name"] = "Lotfi";
    new_data["age"] = 23;
    new_data["random_float"] = 5.95;

    db.putData("person3", new_data);
    // vTaskDelay(500/portTICK_PERIOD_MS);

    // Edit person2 data in the database by patching
    data["age"] = 23;
    db.patchData("person2", data);
    Json::Value root = db.getData("person3"); // retrieve person3 from database, set it to "" to get entire database

    Json::FastWriter writer;
    std::string person3_string = writer.write(root);  // convert it to json string

    ESP_LOGI("MAIN", "person3 as json string: \n%s", person3_string.c_str());

    // You can also print entire Json Value object with std::cout with converting to string 
    // you cant print directly with printf or LOGx because Value objects can have many type. << is overloaded and can print regardless of the type of the Value
    std::cout << root << std::endl;

    // print the members (Value::Members is a vector)
    Json::Value::Members members = root.getMemberNames();  
    for (const auto& member : members)
    {
        std::cout << member << ", ";
    }
    std::cout << std::endl;


    db.deleteData("person3"); // delete person3
    root = db.getData("person3"); // retrieve person3 from database, this time it will be null because person3 doesnt exist in database
    std::cout << root << std::endl;
}


extern "C" void app_main(void)
{
    //wifiInit(SSID, PASSWORD);  // blocking until it connects

    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    
}

