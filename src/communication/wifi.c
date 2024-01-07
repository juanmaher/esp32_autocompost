/**
 * @file wifi.c
 * @brief This module includes functions for handling Wi-Fi events, managing Wi-Fi credentials using NVS,
 * and starting a SmartConfig task for automatic Wi-Fi setup.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

#include "common/events.h"
#include "communication/wifi.h"

#define DEBUG false

// Event base for internal Wi-Fi events
ESP_EVENT_DEFINE_BASE(WIFI_EVENT_INTERNAL);

static const char *TAG = "AC_Wifi";

// Variables to track Wi-Fi connection status and SmartConfig task status
static bool wifi_connected = false;
static bool smartconfig_task_running = false;
static uint32_t connection_failed_counter = 0;

// Event group to signal Wi-Fi events
static EventGroupHandle_t s_wifi_event_group;

// Structure to hold Wi-Fi credentials
static struct wifi_credentials {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_credentials_t;

// Forward declarations of functions
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void smartconfig_task(void * parm);
esp_err_t save_wifi_credentials(const uint8_t *ssid, size_t ssid_length, const uint8_t *password, size_t password_length);
esp_err_t load_wifi_credentials(uint8_t *ssid, uint8_t *password);
size_t automatical_connection(uint8_t *ssid, uint8_t *password);

/**
 * @brief Function to start Wi-Fi module.
 */
void Wifi_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Initialize the network interface and create a default Wi-Fi station interface
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Create the event group to handle Wi-Fi events
    s_wifi_event_group = xEventGroupCreate();

    // Initialize Wi-Fi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register Wi-Fi event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    // Set Wi-Fi mode to station mode and start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * @brief Wi-Fi event handler function.
 *
 * Handles various Wi-Fi events, such as connection status changes and SmartConfig events.
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    ESP_LOGI(TAG, "Event received: %s, %lu", event_base, event_id);

    // Handle Wi-Fi start event
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Try to load Wi-Fi credentials from NVS
        if (load_wifi_credentials(wifi_credentials_t.ssid, wifi_credentials_t.password) == ESP_OK) {
            if (DEBUG) ESP_LOGI(TAG, "Loaded wifi credentials from NVS");
            if (DEBUG) ESP_LOGI(TAG, "SSID:%s", wifi_credentials_t.ssid);
            if (DEBUG) ESP_LOGI(TAG, "PASSWORD:%s", wifi_credentials_t.password);
            
            // Attempt automatic connection using loaded credentials
            if (automatical_connection(wifi_credentials_t.ssid, wifi_credentials_t.password) == ESP_ERR_WIFI_SSID) {
                if (DEBUG) ESP_LOGI(TAG, "Automatic connection failed");
                // Start SmartConfig task if automatic connection fails
                xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
            }
        }
    } 
    // Handle Wi-Fi connected event
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        if (!wifi_connected && !smartconfig_task_running) {
            wifi_connected = true;
            esp_event_post(WIFI_EVENT_INTERNAL, WIFI_EVENT_CONNECTION_ON, NULL, 0, portMAX_DELAY);
        }
    } 
    // Handle Wi-Fi disconnected event
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Start SmartConfig task if not already running
        if (!smartconfig_task_running) {
            xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
        }

        // Process Wi-Fi disconnection
        if (wifi_connected) {
            wifi_connected = false;
            esp_event_post(WIFI_EVENT_INTERNAL, WIFI_EVENT_CONNECTION_OFF, NULL, 0, portMAX_DELAY);
            connection_failed_counter++;
        }

        // Attempt to reconnect to Wi-Fi
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } 
    // Handle IP address acquisition event
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } 
    // Handle SmartConfig scan done event
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } 
    // Handle SmartConfig found channel event
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } 
    // Handle SmartConfig got SSID and password event
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        // Extract SSID and password from event data
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };

        // Copy data from SmartConfig event to Wi-Fi configuration
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        // Copy SSID and password to local variables
        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));

        // Save SSID and password to NVS
        ESP_ERROR_CHECK(save_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password)));

        if (DEBUG) ESP_LOGI(TAG, "SSID:%s", ssid);
        if (DEBUG) ESP_LOGI(TAG, "PASSWORD:%s", password);

        // Disconnect from current Wi-Fi network and connect using SmartConfig credentials
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    } 
    // Handle SmartConfig send ACK done event
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
}

/**
 * @brief Function for the SmartConfig task.
 *
 * This task is responsible for running SmartConfig to automatically configure Wi-Fi credentials.
 *
 * @param parm  Pointer to user data (not used).
 */
static void smartconfig_task(void *parm) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Set SmartConfig type to ESPTOUCH
    smartconfig_task_running = true;
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));

    // Start SmartConfig
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

    EventBits_t uxBits;

    // Wait for Wi-Fi connected or fail event
    while (true) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, true, false, portMAX_DELAY);

        // Process Wi-Fi connected event
        if (uxBits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to AP");
            wifi_connected = true;
            esp_event_post(WIFI_EVENT_INTERNAL, WIFI_EVENT_CONNECTION_ON, NULL, 0, portMAX_DELAY);
        }

        // Process SmartConfig fail event
        if (uxBits & WIFI_FAIL_BIT) {
            ESP_LOGI(TAG, "SmartConfig over");
            esp_smartconfig_stop();
            smartconfig_task_running = false;
            vTaskDelete(NULL);
        }
    }
}

/**
 * @brief Function to save Wi-Fi credentials to NVS.
 *
 * @param ssid              Pointer to the SSID.
 * @param ssid_length       Length of the SSID.
 * @param password          Pointer to the password.
 * @param password_length   Length of the password.
 *
 * @return ESP_OK on success, or error code on failure.
 */
esp_err_t save_wifi_credentials(const uint8_t *ssid, size_t ssid_length, const uint8_t *password, size_t password_length) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Open NVS handle
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        return err;
    }

    // Save SSID to NVS
    err = nvs_set_blob(my_handle, "ssid", ssid, ssid_length);
    if (err != ESP_OK) return err;

    // Save password to NVS
    err = nvs_set_blob(my_handle, "password", password, password_length);
    if (err != ESP_OK) return err;

    // Commit changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close NVS handle
    nvs_close(my_handle);

    return ESP_OK;
}

/**
 * @brief Function to load Wi-Fi credentials from NVS.
 *
 * @param ssid      Pointer to the SSID buffer.
 * @param password  Pointer to the password buffer.
 *
 * @return ESP_OK on success, or error code on failure.
 */
esp_err_t load_wifi_credentials(uint8_t *ssid, uint8_t *password) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Open NVS handle
    nvs_handle_t my_handle;
    size_t ssid_length;
    size_t password_length;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        return err;
    }

    // Get SSID length from NVS
    err = nvs_get_blob(my_handle, "ssid", NULL, &ssid_length);
    if (err == ESP_OK && ssid_length > 0) {
        // Get SSID data from NVS
        err = nvs_get_blob(my_handle, "ssid", ssid, &ssid_length);
        if (err != ESP_OK) return err;
    }

    // Get password length from NVS
    err = nvs_get_blob(my_handle, "password", NULL, &password_length);
    if (err == ESP_OK && password_length > 0) {
        // Get password data from NVS
        err = nvs_get_blob(my_handle, "password", password, &password_length);
        if (err != ESP_OK) return err;
    }

    // Close NVS handle
    nvs_close(my_handle);

    return ESP_OK;
}

/**
 * @brief Function to automatically connect to Wi-Fi using provided credentials.
 *
 * @param ssid      Pointer to the SSID buffer.
 * @param password  Pointer to the password buffer.
 *
 * @return ESP_OK on success, or ESP_ERR_WIFI_SSID if the provided SSID is incorrect.
 */
size_t automatical_connection(uint8_t *ssid, uint8_t *password) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    // Configure Wi-Fi settings
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    // Set Wi-Fi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Attempt to connect to Wi-Fi
    int ret = esp_wifi_connect();

    // Handle incorrect SSID error
    if (ret == ESP_ERR_WIFI_SSID) {
        ESP_LOGI(TAG, "SSID is incorrect");
    }
    ESP_LOGI(TAG, "Connect result: %d", ret);

    return ret;
}
