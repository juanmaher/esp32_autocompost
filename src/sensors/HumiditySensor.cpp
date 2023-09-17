#include "sensors/HumiditySensor.hpp"

#define TIMER_EXPIRED_BIT (1 << 0)

TimerHandle_t estableTimer;
EventGroupHandle_t humiditySensorEventGroup;
int timerId;

void vTimerCallback(TimerHandle_t pxTimer) {
    printf("Timer elapsed\n");
    xEventGroupSetBits(humiditySensorEventGroup, TIMER_EXPIRED_BIT);
}

/*

Ver el tema de cambiar de tiempos

mandar eventos cuando hay algo inestable

se diferencia o no la temperatra de la humedad ante inestabilidad?


*/

void DHT_reader_task(void *pvParameter) {
    setDHTgpio(GPIO_NUM_27);
    printf("reader task");
    while (1) {
        printf("wait\n");
        
        // Esperar a que el evento del temporizador se active
        EventBits_t uxBits = xEventGroupWaitBits(humiditySensorEventGroup, TIMER_EXPIRED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        
        if (uxBits & TIMER_EXPIRED_BIT) {
            printf("DHT Sensor Readings\n");
            int ret = readDHT();

            errorHandler(ret);

            printf("Humidity %.2f %%\n", getHumidity());
            printf("Temperature %.2f degC\n\n", getTemperature());
        }
        
    }

    vTaskDelete(NULL);
}

HumiditySensor::HumiditySensor() {
    printf("constructor\n");
    humiditySensorEventGroup = xEventGroupCreate();
    estableTimer = xTimerCreate("Timer", pdMS_TO_TICKS(5000), pdTRUE, (void *)timerId, vTimerCallback);
}

void HumiditySensor::start() {
    printf("start\n");
    xTaskCreate(DHT_reader_task, "DHT_reader_task", 2048, NULL, 5, NULL);
    xTimerStart(estableTimer, 0); // Inicia el temporizador cuando comienza la tarea
}



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