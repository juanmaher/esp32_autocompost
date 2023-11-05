#include "sensors/humidity_sensor.h"

#define TIMER_EXPIRED_BIT           (1 << 0)
#define STABLE_HUMIDITY_TIMER      6000
#define UNSTABLE_HUMIDITY_TIMER     1000
#define MAX_HUMIDITY                60

static const char *TAG = "AC_HumiditySensor";

static HumiditySensor_t sensor;

static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(sensor.eventGroup, TIMER_EXPIRED_BIT);
}

static void reader_task(void *pvParameter) {
    EventBits_t uxBits;
    float humidity;
    setDHTgpio(HUMIDITY_SENSOR_GPIO);

    while (1) {   
        uxBits = xEventGroupWaitBits(sensor.eventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);
        
        if (uxBits & TIMER_EXPIRED_BIT) {
            //ESP_LOGI(TAG, "Reading values:");
            int ret = readDHT();
            errorHandler(ret);

            humidity = getHumidity();
            ESP_LOGI(TAG,"Humidity %.2f %%", humidity);
            //ESP_LOGI(TAG,"Temperature %.2f degC", getTemperature());

            if (humidity > MAX_HUMIDITY) {
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(UNSTABLE_HUMIDITY_TIMER), portMAX_DELAY);
            } else {
                xTimerChangePeriod(sensor.stableTimer, pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER), portMAX_DELAY);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

void HumiditySensor_Start() {
    sensor.eventGroup = xEventGroupCreate(); 
    sensor.stableTimer = xTimerCreate("HumidiySensor_Timer", pdMS_TO_TICKS(STABLE_HUMIDITY_TIMER), true, NULL, timer_callback);
    xTaskCreate(reader_task, "HumiditySensor_ReaderTask", 2048, NULL, 4, NULL);
    xTimerStart(sensor.stableTimer, 0);
}