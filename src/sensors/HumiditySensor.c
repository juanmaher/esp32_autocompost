#include "sensors/HumiditySensor.h"

#define TIMER_EXPIRED_BIT           (1 << 0)
#define HUMIDITY_SENSOR_GPIO_PIN    GPIO_NUM_27

static const char *TAG = "AC_HumiditySensor";

static void timer_callback(TimerHandle_t pxTimer) {
    HumiditySensor_t *sensor = (HumiditySensor_t *)pvTimerGetTimerID(pxTimer);
    xEventGroupSetBits(sensor->humiditySensorEventGroup, TIMER_EXPIRED_BIT);
}

static void reader_task(void *pvParameter) {
    HumiditySensor_t *sensor = (HumiditySensor_t *)pvParameter;
    EventBits_t uxBits;

    setDHTgpio(HUMIDITY_SENSOR_GPIO_PIN);

    while (1) {   
        uxBits = xEventGroupWaitBits(sensor->humiditySensorEventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);
        
        if (uxBits & TIMER_EXPIRED_BIT) {
            ESP_LOGI(TAG, "Reading values:");
            int ret = readDHT();

            errorHandler(ret);

            ESP_LOGI(TAG,"Humidity %.2f %%", getHumidity());
            ESP_LOGI(TAG,"Temperature %.2f degC", getTemperature());
        }
    }
    vTaskDelete(NULL);
}

void HumiditySensor_Init(HumiditySensor_t *sensor) {
    sensor->humiditySensorEventGroup = xEventGroupCreate(); 
    sensor->estableTimer = xTimerCreate("HumidiySensor_Timer", pdMS_TO_TICKS(5000), true, (void *)sensor, timer_callback);
}

void HumiditySensor_Start(HumiditySensor_t *sensor) {
    xTaskCreate(reader_task, "HumiditySensor_ReaderTask", 2048, (void *)sensor, 5, NULL);
    xTimerStart(sensor->estableTimer, 0);
}