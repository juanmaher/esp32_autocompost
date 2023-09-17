#include "sensors/HumiditySensor.hpp"

#define TIMER_EXPIRED_BIT           (1 << 0)
#define HUMIDITY_SENSOR_GPIO_PIN    GPIO_NUM_27

static const char *TAG = "AC_HumiditySensor";

TimerHandle_t estableTimer;
EventGroupHandle_t humiditySensorEventGroup;
int timerId;

void vTimerCallback(TimerHandle_t pxTimer) {
    xEventGroupSetBits(humiditySensorEventGroup, TIMER_EXPIRED_BIT);
}

void DHT_reader_task(void *pvParameter) {
    setDHTgpio(HUMIDITY_SENSOR_GPIO_PIN);

    while (1) {   
        EventBits_t uxBits = xEventGroupWaitBits(humiditySensorEventGroup, TIMER_EXPIRED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        
        if (uxBits & TIMER_EXPIRED_BIT) {
            ESP_LOGI(TAG, "Reading values:");
            int ret = readDHT();

            errorHandler(ret);

            ESP_LOGI(TAG, "Humidity %.2f %%", getHumidity());
            ESP_LOGI(TAG,"Temperature %.2f degC", getTemperature());
        }
    }
    vTaskDelete(NULL);
}

HumiditySensor::HumiditySensor() {
    humiditySensorEventGroup = xEventGroupCreate();
    estableTimer = xTimerCreate("Timer", pdMS_TO_TICKS(5000), pdTRUE, (void *)timerId, vTimerCallback);
}

void HumiditySensor::start() {
    xTaskCreate(DHT_reader_task, "DHT_reader_task", 2048, NULL, 5, NULL);
    xTimerStart(estableTimer, 0); // Inicia el temporizador cuando comienza la tarea
}



/*

Ver el tema de cambiar de tiempos

mandar eventos cuando hay algo inestable

se diferencia o no la temperatra de la humedad ante inestabilidad?


*/


/*
#include "sensors/HumiditySensor.hpp"

TimerHandle_t estableTimer;
bool timerExpired = false; // Variable compartida

void vTimerCallback(TimerHandle_t pxTimer) {
    printf("Se llamó al temporizador\n");
    timerExpired = true; // Establecer la variable como verdadera
}

void DHT_reader_task(void *pvParameter) {
    setDHTgpio(GPIO_NUM_27);
    printf("reader task");
    while (1) {
        printf("wait");
        
        // Esperar hasta que el temporizador haya expirado
        while (!timerExpired) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Espera corta
        }
        
        // Reiniciar la variable
        timerExpired = false;

        printf("DHT Sensor Readings\n");
        int ret = readDHT();

        errorHandler(ret);

        printf("Humidity %.2f %%\n", getHumidity());
        printf("Temperature %.2f degC\n\n", getTemperature());

        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    vTaskDelete(NULL);
}

HumiditySensor::HumiditySensor() {
    printf("constructor\n");
    estableTimer = xTimerCreate("Timer", pdMS_TO_TICKS(5000), pdTRUE, NULL, vTimerCallback);
}

void HumiditySensor::start() {
    printf("start\n");
    xTaskCreate(DHT_reader_task, "DHT_reader_task", 2048, NULL, 5, NULL);
    xTimerStart(estableTimer, 0); // Inicia el temporizador cuando comienza la tarea
}

*/