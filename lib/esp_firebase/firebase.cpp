#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "cJSON.h"
#include "firebase.hpp"

#define NVS_TAG "NVS"
#define HTTP_TAG "HTTP_CLIENT"
#define FIREBASE_APP_TAG "FirebaseApp"
#define DEBUG false

extern const char cert_start[] asm("_binary_gtsr1_pem_start");

static int output_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id){
        case HTTP_EVENT_ERROR:
            if (DEBUG) if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
            memset(evt->user_data, 0, HTTP_RECV_BUFFER_SIZE);
            output_len = 0;
            break;
        case HTTP_EVENT_HEADER_SENT:
            if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_FINISH:
            if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
            output_len = 0;
            break;
        case HTTP_EVENT_ON_DATA:
            if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            memcpy((char *)evt->user_data + output_len, evt->data, evt->data_len);
            output_len += evt->data_len;
            break;
        case HTTP_EVENT_DISCONNECTED:
            if (DEBUG) ESP_LOGI(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void FirebaseApp::firebaseClientInit(void) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    esp_http_client_config_t config = {
        .url = "https://google.com",        // Debes configurar esto como un enlace HTTPS válido
        .cert_pem = FirebaseApp::https_certificate,
        .event_handler = http_event_handler,
        .buffer_size = HTTP_RECV_BUFFER_SIZE,
        .buffer_size_tx = 4096,
        .user_data = FirebaseApp::local_response_buffer
    };

    FirebaseApp::client = esp_http_client_init(&config);
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "HTTP Client Initialized");
}

esp_err_t FirebaseApp::setHeader(const char* header, const char* value) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    return esp_http_client_set_header(FirebaseApp::client, header, value);
}

http_ret_t FirebaseApp::performRequest(const char* url, esp_http_client_method_t method, std::string post_field) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    ESP_ERROR_CHECK(esp_http_client_set_url(FirebaseApp::client, url));
    ESP_ERROR_CHECK(esp_http_client_set_method(FirebaseApp::client, method));
    ESP_ERROR_CHECK(esp_http_client_set_post_field(FirebaseApp::client, post_field.c_str(), post_field.length()));
    esp_err_t err = esp_http_client_perform(FirebaseApp::client);
    int status_code = esp_http_client_get_status_code(FirebaseApp::client);
    if (err != ESP_OK || status_code != 200)
    {
        ESP_LOGE(FIREBASE_APP_TAG, "Error while performing request esp_err_t code=0x%x | status_code=%d", (int)err, status_code);
        ESP_LOGE(FIREBASE_APP_TAG, "request: url=%s \nmethod=%d \npost_field=%s", url, method, post_field.c_str());
        ESP_LOGE(FIREBASE_APP_TAG, "response=\n%s", local_response_buffer);
    }
    return {err, status_code};
}

void FirebaseApp::clearHTTPBuffer(void) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    memset(FirebaseApp::local_response_buffer, 0, HTTP_RECV_BUFFER_SIZE);
    output_len = 0;
}

esp_err_t FirebaseApp::getRefreshToken(bool register_account) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    http_ret_t http_ret;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "email", FirebaseApp::user_account.user_email);
    cJSON_AddStringToObject(root, "password", FirebaseApp::user_account.user_password);
    cJSON_AddBoolToObject(root, "returnSecureToken", true);

    char *account_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    FirebaseApp::setHeader("content-type", "application/json");
    if (register_account) {
        http_ret = FirebaseApp::performRequest(FirebaseApp::register_url.c_str(), HTTP_METHOD_POST, account_json);
    } else {
        http_ret = FirebaseApp::performRequest(FirebaseApp::login_url.c_str(), HTTP_METHOD_POST, account_json);
    }

    free(account_json);

    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        const char *begin = FirebaseApp::local_response_buffer;
        cJSON *root = cJSON_Parse(begin);
        if (root) {
            cJSON *refreshToken = cJSON_GetObjectItemCaseSensitive(root, "refreshToken");
            if (refreshToken && cJSON_IsString(refreshToken)) {
                FirebaseApp::refresh_token = refreshToken->valuestring;
                if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "Refresh Token=%s", FirebaseApp::refresh_token.c_str());
                cJSON_Delete(root);
                return ESP_OK;
            }
            cJSON_Delete(root);
        }
    }

    return ESP_FAIL;
}

esp_err_t FirebaseApp::getAuthToken() {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    http_ret_t http_ret;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "grant_type", "refresh_token");
    cJSON_AddStringToObject(root, "refresh_token", FirebaseApp::refresh_token.c_str());

    char *token_post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    FirebaseApp::setHeader("content-type", "application/json");
    http_ret = FirebaseApp::performRequest(FirebaseApp::auth_url.c_str(), HTTP_METHOD_POST, token_post_data);
    free(token_post_data);

    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        const char *begin = FirebaseApp::local_response_buffer;
        cJSON *root = cJSON_Parse(begin);
        if (root) {
            cJSON *accessToken = cJSON_GetObjectItemCaseSensitive(root, "access_token");
            if (accessToken && cJSON_IsString(accessToken)) {
                FirebaseApp::auth_token = accessToken->valuestring;
                if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "Auth Token=%s", FirebaseApp::auth_token.c_str());
                cJSON_Delete(root);
                return ESP_OK;
            }
            cJSON_Delete(root);
        }
    }

    return ESP_FAIL;
}

FirebaseApp::FirebaseApp(const char *api_key) : https_certificate(cert_start), api_key(api_key) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    FirebaseApp::local_response_buffer = (char *)malloc(HTTP_RECV_BUFFER_SIZE);
    FirebaseApp::register_url += FirebaseApp::api_key;
    FirebaseApp::login_url += FirebaseApp::api_key;
    FirebaseApp::auth_url += FirebaseApp::api_key;
    firebaseClientInit();
}

FirebaseApp::~FirebaseApp() {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    free(FirebaseApp::local_response_buffer);
    esp_http_client_cleanup(FirebaseApp::client);
}

esp_err_t FirebaseApp::registerUserAccount(const user_account_t &account) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    if (FirebaseApp::user_account.user_email != account.user_email || FirebaseApp::user_account.user_password != account.user_password) {
        FirebaseApp::user_account.user_email = account.user_email;
        FirebaseApp::user_account.user_password = account.user_password;
    }

    esp_err_t err = FirebaseApp::getRefreshToken(true);
    if (err != ESP_OK) {
        ESP_LOGE(FIREBASE_APP_TAG, "Failed to get refresh token");
        return ESP_FAIL;
    }

    FirebaseApp::clearHTTPBuffer();
    err = FirebaseApp::getAuthToken();
    
    if (err != ESP_OK) {
        ESP_LOGE(FIREBASE_APP_TAG, "Failed to get auth token");
        return ESP_FAIL;
    }

    FirebaseApp::clearHTTPBuffer();
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "Created user successfully");

    return ESP_OK;
}

esp_err_t FirebaseApp::loginUserAccount(const user_account_t &account) {
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "on %s", __func__);

    if (FirebaseApp::user_account.user_email != account.user_email || FirebaseApp::user_account.user_password != account.user_password) {
        FirebaseApp::user_account.user_email = account.user_email;
        FirebaseApp::user_account.user_password = account.user_password;
    }

    esp_err_t err = FirebaseApp::getRefreshToken(false);

    if (err != ESP_OK) {
        ESP_LOGE(FIREBASE_APP_TAG, "Failed to get refresh token");
        return ESP_FAIL;
    }

    FirebaseApp::clearHTTPBuffer();
    err = FirebaseApp::getAuthToken();

    if (err != ESP_OK) {
        ESP_LOGE(FIREBASE_APP_TAG, "Failed to get auth token");
        return ESP_FAIL;
    }

    FirebaseApp::clearHTTPBuffer();
    if (DEBUG) ESP_LOGI(FIREBASE_APP_TAG, "Login to user successful");
    return ESP_OK;
}
