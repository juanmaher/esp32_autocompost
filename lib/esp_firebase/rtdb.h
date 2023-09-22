#ifndef _ESP_FIREBASE_RTDB_H_
#define  _ESP_FIREBASE_RTDB_H_

#include "firebase.hpp"
#include "cJSON.h"

class RTDB {
    private:
        FirebaseApp* app;
        std::string base_database_url;

    public:
        RTDB();
        RTDB(FirebaseApp* app, const char* database_url);
        void initialize(FirebaseApp* app, const char* database_url);

        cJSON* getData(const char* path);

        esp_err_t putData(const char* path, const char* json_str);
        esp_err_t putData(const char* path, cJSON* data_json);

        esp_err_t postData(const char* path, const char* json_str);
        esp_err_t postData(const char* path, cJSON* data_json);

        esp_err_t patchData(const char* path, const char* json_str);
        esp_err_t patchData(const char* path, cJSON* data_json);

        esp_err_t deleteData(const char* path);
};

#endif
