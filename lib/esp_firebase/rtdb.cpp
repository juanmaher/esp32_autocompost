#include <iostream>
#include "esp_log.h"

#include "cJSON.h"
#include "firebase.hpp"
#include "rtdb.h"

#define RTDB_TAG "RTDB"
#define DEBUG false

RTDB::RTDB() {
    app = nullptr;
    base_database_url = "";
}

/**
 * Initializes the RTDB class with the given Firebase app and database URL.
 *
 * @param app A pointer to the FirebaseApp object.
 * @param database_url The URL of the database to be connected to.
 *
 * @throws N/A
 */
void RTDB::initialize(FirebaseApp* app, const char* database_url) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    this->app = app;
    this->base_database_url = database_url;
}

RTDB::RTDB(FirebaseApp* app, const char* database_url) : app(app), base_database_url(database_url) { }

/**
 * Retrieves data from the Real-Time Database at the specified path.
 *
 * @param path The path of the data to retrieve.
 *
 * @return A JSON object containing the retrieved data.
 *
 * @throws esp_err_t An error occurred while retrieving the data.
 */
cJSON* RTDB::getData(const char* path) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    std::string url = RTDB::base_database_url;
    url += path;
    url += ".json?auth=" + this->app->auth_token;

    this->app->setHeader("content-type", "application/json");
    http_ret_t http_ret = this->app->performRequest(url.c_str(), HTTP_METHOD_GET, "");

    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        const char* begin = this->app->local_response_buffer;

        cJSON* data_json = cJSON_Parse(begin);
        ESP_LOGI(RTDB_TAG, "Data with path=%s acquired", path);
        this->app->clearHTTPBuffer();
        return data_json;
    } else {
        ESP_LOGE(RTDB_TAG, "Error while getting data at path %s| esp_err_t=%d | status_code=%d", path, (int)http_ret.err, http_ret.status_code);
        ESP_LOGI(RTDB_TAG, "Token expired ? Trying refreshing auth");
        this->app->setHeader("content-type", "application/json");
        http_ret = this->app->performRequest(url.c_str(), HTTP_METHOD_GET, "");

        if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
            const char* begin = this->app->local_response_buffer;

            cJSON* data_json = cJSON_Parse(begin);
            ESP_LOGI(RTDB_TAG, "Data with path=%s acquired", path);
            this->app->clearHTTPBuffer();
            return data_json;
        } else {
            ESP_LOGE(RTDB_TAG, "Failed to get data after refreshing token. double check account credentials or database rules");
            this->app->clearHTTPBuffer();
            return NULL;
        }
    }
}

/**
 * Puts data into the Real-Time Database at the specified path.
 *
 * @param path The path in the database where the data should be stored.
 * @param json_str The JSON string representing the data to be stored.
 *
 * @return An `esp_err_t` value indicating the success or failure of the operation.
 *
 * @throws None.
 */
esp_err_t RTDB::putData(const char* path, const char* json_str) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    std::string url = RTDB::base_database_url;
    url += path;
    url += ".json?auth=" + this->app->auth_token;
    this->app->setHeader("content-type", "application/json");
    http_ret_t http_ret = this->app->performRequest(url.c_str(), HTTP_METHOD_PUT, json_str);
    this->app->clearHTTPBuffer();
    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        ESP_LOGI(RTDB_TAG, "PUT successful");
        return ESP_OK;
    } else {
        ESP_LOGE(RTDB_TAG, "PUT failed");
        return ESP_FAIL;
    }
}

/**
 * Puts data into the RTDB at the given path with the provided JSON data.
 *
 * @param path The path to store the data.
 * @param data_json The JSON data to be stored.
 *
 * @return The error status of the operation.
 *
 * @throws esp_err_t An error occurred while putting the data.
 */
esp_err_t RTDB::putData(const char* path, cJSON* data_json) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    char* json_str = cJSON_PrintUnformatted(data_json);
    esp_err_t err = RTDB::putData(path, json_str);
    cJSON_free(json_str);
    return err;
}

/**
 * Posts data to the Real-Time Database.
 *
 * @param path The path where the data should be posted.
 * @param json_str The JSON string to be posted.
 *
 * @return An `esp_err_t` indicating the status of the post operation.
 *
 * @throws None
 */
esp_err_t RTDB::postData(const char* path, const char* json_str) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    std::string url = RTDB::base_database_url;
    url += path;
    url += ".json?auth=" + this->app->auth_token;
    this->app->setHeader("content-type", "application/json");
    http_ret_t http_ret = this->app->performRequest(url.c_str(), HTTP_METHOD_POST, json_str);
    this->app->clearHTTPBuffer();
    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        ESP_LOGI(RTDB_TAG, "POST successful");
        return ESP_OK;
    } else {
        ESP_LOGE(RTDB_TAG, "POST failed");
        return ESP_FAIL;
    }
}

/**
 * Posts data to the specified path in the real-time database.
 *
 * @param path The path to where the data will be posted.
 * @param data_json The JSON data to be posted.
 *
 * @return The error code indicating the success or failure of the operation.
 *
 * @throws esp_err_t An error occurred while posting the data.
 */
esp_err_t RTDB::postData(const char* path, cJSON* data_json) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    char* json_str = cJSON_PrintUnformatted(data_json);
    esp_err_t err = RTDB::postData(path, json_str);
    cJSON_free(json_str);
    return err;
}

/**
 * Patches data at the specified path in the Real-Time Database.
 *
 * @param path The path where the data will be patched.
 * @param json_str The JSON string representing the data to be patched.
 *
 * @return An `esp_err_t` indicating the result of the patch operation.
 *         - `ESP_OK` if the patch operation was successful.
 *         - `ESP_FAIL` if the patch operation failed.
 *
 * @throws None
 */
esp_err_t RTDB::patchData(const char* path, const char* json_str) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    std::string url = RTDB::base_database_url;
    url += path;
    url += ".json?auth=" + this->app->auth_token;
    this->app->setHeader("content-type", "application/json");
    http_ret_t http_ret = this->app->performRequest(url.c_str(), HTTP_METHOD_PATCH, json_str);
    this->app->clearHTTPBuffer();
    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        ESP_LOGI(RTDB_TAG, "PATCH successful");
        return ESP_OK;
    } else {
        ESP_LOGE(RTDB_TAG, "PATCH failed");
        return ESP_FAIL;
    }
}

/**
 * Patches the data at the specified path with the provided JSON data.
 *
 * @param path The path where the data should be patched.
 * @param data_json The JSON data to be patched.
 *
 * @return An esp_err_t indicating the success or failure of the patch operation.
 *
 * @throws None.
 */
esp_err_t RTDB::patchData(const char* path, cJSON* data_json) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    char* json_str = cJSON_PrintUnformatted(data_json);
    esp_err_t err = RTDB::patchData(path, json_str);
    cJSON_free(json_str);
    return err;
}

/**
 * Deletes data from the Real-Time Database.
 *
 * @param path the path where the data is located
 *
 * @return ESP_OK if the data is successfully deleted, ESP_FAIL otherwise
 *
 * @throws None
 */
esp_err_t RTDB::deleteData(const char* path) {
    if (DEBUG) ESP_LOGI(RTDB_TAG, "on %s", __func__);

    std::string url = RTDB::base_database_url;
    url += path;
    url += ".json?auth=" + this->app->auth_token;
    this->app->setHeader("content-type", "application/json");
    http_ret_t http_ret = this->app->performRequest(url.c_str(), HTTP_METHOD_DELETE, "");
    this->app->clearHTTPBuffer();
    if (http_ret.err == ESP_OK && http_ret.status_code == 200) {
        ESP_LOGI(RTDB_TAG, "DELETE successful");
        return ESP_OK;
    } else {
        ESP_LOGE(RTDB_TAG, "DELETE failed");
        return ESP_FAIL;
    }
}
