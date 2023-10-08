#include "sensors/HumiditySensor.h"

#define TIMER_EXPIRED_BIT           (1 << 0)
#define HUMIDITY_SENSOR_GPIO_PIN    GPIO_NUM_27
#define HUMIDITY_MEASURE_DELAY      5000

static const char *TAG = "AC_HumiditySensor";

static HumiditySensor_t humiditySensor;

static void timer_callback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(humiditySensor.humiditySensorEventGroup, TIMER_EXPIRED_BIT);
}

static void reader_task(void *pvParameter) {
    EventBits_t uxBits;

    setDHTgpio(HUMIDITY_SENSOR_GPIO_PIN);

    while (1) {   
        uxBits = xEventGroupWaitBits(humiditySensor.humiditySensorEventGroup, TIMER_EXPIRED_BIT, true, false, portMAX_DELAY);
        
        if (uxBits & TIMER_EXPIRED_BIT) {
            //ESP_LOGI(TAG, "Reading values:");
            int ret = readDHT();

            errorHandler(ret);

            ESP_LOGI(TAG,"Humidity %.2f %%", getHumidity());
            //ESP_LOGI(TAG,"Temperature %.2f degC", getTemperature());
        }
    }
    vTaskDelete(NULL);
}

void HumiditySensor_Start() {
    humiditySensor.humiditySensorEventGroup = xEventGroupCreate(); 
    humiditySensor.estableTimer = xTimerCreate("HumidiySensor_Timer", pdMS_TO_TICKS(HUMIDITY_MEASURE_DELAY), true, NULL, timer_callback);
    xTaskCreate(reader_task, "HumiditySensor_ReaderTask", 2048, NULL, 5, NULL);
    xTimerStart(humiditySensor.estableTimer, 0);
}