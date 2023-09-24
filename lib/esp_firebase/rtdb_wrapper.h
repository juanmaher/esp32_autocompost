#ifndef RTDB_WRAPPER_H_
#define RTDB_WRAPPER_H_

#include "cJSON.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* user_email;
    const char* user_password;
} user_data_t;

typedef struct _RTDB_t {
    int (* const initialize)        (struct _RTDB_t *me, const char * api_key, user_data_t account, const char* database_url);
    cJSON * (* const getData)       (struct _RTDB_t *me, const char* path);
    int (* const putData)           (struct _RTDB_t *me, const char* path, const char* json_str);
    int (* const putDataJson)       (struct _RTDB_t *me, const char* path, cJSON* data_json);
    int (* const postData)          (struct _RTDB_t *me, const char* path, const char* json_str);
    int (* const patchData)         (struct _RTDB_t *me, const char* path, const char* json_str);
    int (* const deleteData)        (struct _RTDB_t *me, const char* path, const char* json_str);

    void * const obj;
} RTDB_t;

RTDB_t* RTDB_Create(const char * api_key, user_data_t account, const char* database_url);
void RTDB_Destroy(RTDB_t *me);

#ifdef __cplusplus
}
#endif

#endif
