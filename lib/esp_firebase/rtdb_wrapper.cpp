#include "rtdb_wrapper.h"
#include "rtdb.h"
#include "firebase.hpp"
#include "esp_log.h"
#include "freertos/semphr.h"
#define DEBUG false

static const char *TAG = "RTDB_Wrapper";

int RTDB_Initialize(RTDB_t* me, const char * api_key, user_account_t account, const char* database_url);
cJSON * RTDB_GetData(RTDB_t* me, const char* path);
int RTDB_PutData(RTDB_t* me, const char* path, const char* json_str);
int RTDB_PutDataJson(RTDB_t* me, const char* path, cJSON* data_json);
int RTDB_PostData(RTDB_t* me, const char* path, const char* json_str);
int RTDB_PatchData(RTDB_t* me, const char* path, const char* json_str);
int RTDB_PatchDataJson(RTDB_t* me, const char* path, cJSON* data_json);
int RTDB_DeleteData(RTDB_t* me, const char* path);
static user_account_t convert_to_user_account(user_data_t data);

FirebaseApp* globalFirebaseApp = NULL;
SemaphoreHandle_t rtdbMutex;

RTDB_t* RTDB_Create(const char * api_key, user_data_t account, const char* database_url) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    if (globalFirebaseApp == NULL) {
        globalFirebaseApp = new FirebaseApp(api_key);
        ESP_ERROR_CHECK(globalFirebaseApp->loginUserAccount(convert_to_user_account(account)));
    }

    RTDB_t* me = (RTDB_t*)malloc(sizeof(RTDB_t));
    if (me) {
        *((void **) &me->obj)           = (void *) new RTDB(globalFirebaseApp, database_url);
        *((void **) &me->initialize)    = (void *) RTDB_Initialize;
        *((void **) &me->getData)       = (void *) RTDB_GetData;
        *((void **) &me->putData)       = (void *) RTDB_PutData;
        *((void **) &me->putDataJson)   = (void *) RTDB_PutDataJson;
        *((void **) &me->postData)      = (void *) RTDB_PostData;
        *((void **) &me->patchData)     = (void *) RTDB_PatchData;
        *((void **) &me->patchDataJson) = (void *) RTDB_PatchDataJson;
        *((void **) &me->deleteData)    = (void *) RTDB_DeleteData;
    }

    rtdbMutex = xSemaphoreCreateMutex();

    return me;
}

void RTDB_Destroy(RTDB_t *me) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    if (me == NULL) {
        return;
    }

    vSemaphoreDelete(rtdbMutex);
    delete static_cast<RTDB *>(me->obj);
    free(globalFirebaseApp);
    free(me);
}

int RTDB_Initialize(RTDB_t* me, const char * api_key, user_account_t account, const char* database_url) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);
    FirebaseApp firebase = FirebaseApp(api_key);
    ESP_ERROR_CHECK(firebase.loginUserAccount(account));

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    obj->initialize(&firebase, database_url);
    xSemaphoreGive(rtdbMutex);

    return ESP_OK;
}

cJSON * RTDB_GetData(RTDB_t* me, const char* path) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    cJSON *data_json = NULL;
    
    if (me == NULL) {
        return NULL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    if (DEBUG) ESP_LOGI(TAG, "on %s: %d", __func__, __LINE__);
    data_json = obj->getData(path);
    if (DEBUG) ESP_LOGI(TAG, "on %s: %d", __func__, __LINE__);
    xSemaphoreGive(rtdbMutex);
    if (DEBUG) ESP_LOGI(TAG, "on %s: %d", __func__, __LINE__);

    if (data_json == NULL) {
        return NULL;
    }

    return data_json;
}

int RTDB_PutData(RTDB_t* me, const char* path, const char* json_str) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    int result = obj->putData(path, json_str);
    xSemaphoreGive(rtdbMutex);

    return result;
}

int RTDB_PutDataJson(RTDB_t* me, const char* path, cJSON* data_json) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    int result = obj->putData(path, data_json);
    xSemaphoreGive(rtdbMutex);

    return result;
}

int RTDB_PostData(RTDB_t* me, const char* path, const char* json_str) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    int result = obj->postData(path, json_str);
    xSemaphoreGive(rtdbMutex);

    return result;
}

int RTDB_PatchData(RTDB_t* me, const char* path, const char* json_str) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    int result = obj->patchData(path, json_str);
    xSemaphoreGive(rtdbMutex);

    return result;
}

int RTDB_PatchDataJson(RTDB_t* me, const char* path, cJSON* data_json) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    int result = obj->patchData(path, data_json);
    xSemaphoreGive(rtdbMutex);

    return result;
}

int RTDB_DeleteData(RTDB_t* me, const char* path) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);
    RTDB *obj;
    if (me == NULL) {
        return ESP_FAIL;
    }

    obj = static_cast<RTDB *>(me->obj);

    xSemaphoreTake(rtdbMutex, portMAX_DELAY);
    int result = obj->deleteData(path);
    xSemaphoreGive(rtdbMutex);

    return result;
}

static user_account_t convert_to_user_account(user_data_t data) {
    user_account_t account;
    account.user_email = data.user_email;
    account.user_password = data.user_password;
    return account;
}
