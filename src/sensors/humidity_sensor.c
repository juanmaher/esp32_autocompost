#include "common/composter_parameters.h"
#include "common/events.h"
#include "sensors/humidity_sensor.h"

#define DEBUG true

#define TIMER_EXPIRED_BIT               (1 << 0)
#define STABLE_HUMIDITY_TIMER_MS        10 * 60 * 1000
#define UNSTABLE_HUMIDITY_TIMER_MS      2 * 60 * 1000
#define MAX_HUMIDITY                    60

ESP_EVENT_DEFINE_BASE(HUMIDITY_EVENT);

static const char *TAG = "AC_HumiditySensor";

static HumiditySensor_t sensor;
extern ComposterParameters composterParameters;

static int sensor_failures = 0;

static void timer_callback(TimerHandle_t pxTimer);
static void reader_task(void *pvParameter);
void reset_humidity_sensor();

static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
}

static void reader_task(void *pvParameter) {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    EventBits_t uxBits;
    float humidity;
    setDHTgpio(HUMIDITY_SENSOR_GPIO);

    while (true) {
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);
        
        if (uxBits & TIMER_EXPIRED_BIT) {
            if (DEBUG) ESP_LOGI(TAG, "Reading values:");
            int ret = readDHT();

            if (ret != DHT_OK) {
                errorHandler(ret);
                sensor_failures++;
                if (sensor_failures >= 5) {
                    reset_humidity_sensor();
                }
                continue;
            }

            humidity = getHumidity();
            if (DEBUG) ESP_LOGI(TAG,"Humidity %.2f %%", humidity);
            //if (DEBUG) ESP_LOGI(TAG,"Temperature %.2f degC", getTemperature());

            ComposterParameters_SetHumidity(&composterParameters, humidity);

            if (humidity > MAX_HUMIDITY) {
                ComposterParameters_SetHumidityState(&composterParameters, false);
                esp_event_post(HUMIDITY_EVENT, HUMIDITY_EVENT_UNSTABLE, NULL, 0, portMAX_DELAY);
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(UNSTABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
            } else {
                ComposterParameters_SetHumidityState(&composterParameters, true);
                esp_event_post(HUMIDITY_EVENT, HUMIDITY_EVENT_STABLE, NULL, 0, portMAX_DELAY);
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), portMAX_DELAY);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

void HumiditySensor_Start() {
    if (DEBUG) ESP_LOGI(TAG, "on %s", __func__);

    sensor.eventGroup = xEventGroupCreate(); 
    sensor.stableTimer = xTimerCreate("HumidiySensor_Timer", pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER_MS), true, NULL, timer_callback);
    xTaskCreate(reader_task, "HumiditySensor_ReaderTask", 2048, NULL, 4, NULL);
    xTimerStart(sensor.stableTimer, 0);
}

void reset_humidity_sensor() {
    setDHTgpio(HUMIDITY_SENSOR_GPIO);
    sensor_failures = 0;
}