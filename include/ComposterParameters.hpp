#ifndef COMPOSTERPARAMETERS_HPP
#define COMPOSTERPARAMETERS_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class ComposterParameters {
    private:
        double complete;
        int days;
        double humidity;
        double temperature;
        bool mixer;
        bool crusher;
        bool fan;

        // Mutex para sincronización
        SemaphoreHandle_t mutex;

    public:
        ComposterParameters() : complete(0.0), days(0), humidity(0.0), temperature(0.0), mixer(false), crusher(false), fan(false) {
            // Crea el mutex
            mutex = xSemaphoreCreateMutex();
        }

        ~ComposterParameters() {
            vSemaphoreDelete(mutex);
        }

        // Métodos para obtener valores (getters) con sincronización
        double getComplete() {
            double result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = complete;
            xSemaphoreGive(mutex);
            return result;
        }

        int getDays() {
            int result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = days;
            xSemaphoreGive(mutex);
            return result;
        }

        double getHumidity() {
            double result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = humidity;
            xSemaphoreGive(mutex);
            return result;
        }

        double getTemperature() {
            double result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = temperature;
            xSemaphoreGive(mutex);
            return result;
        }

        bool getMixerState() {
            bool result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = mixer;
            xSemaphoreGive(mutex);
            return result;
        }

        bool getCrusherState() {
            bool result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = crusher;
            xSemaphoreGive(mutex);
            return result;
        }

        bool getFanState() {
            bool result;
            xSemaphoreTake(mutex, portMAX_DELAY);
            result = fan;
            xSemaphoreGive(mutex);
            return result;
        }

        // Métodos para establecer valores (setters) con sincronización
        void setComplete(double value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            complete = value;
            xSemaphoreGive(mutex);
        }

        void setDays(int value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            days = value;
            xSemaphoreGive(mutex);
        }

        void setHumidity(double value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            humidity = value;
            xSemaphoreGive(mutex);
        }

        void setTemperature(double value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            temperature = value;
            xSemaphoreGive(mutex);
        }

        void setMixerState(bool value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            mixer = value;
            xSemaphoreGive(mutex);
        }

        void setCrusherState(bool value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            crusher = value;
            xSemaphoreGive(mutex);
        }

        void setFanState(bool value) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            fan = value;
            xSemaphoreGive(mutex);
        }
};

#endif // COMPOSTERPARAMETERS_HPP
