#ifndef GPIOS_H
#define GPIOS_H

/* Buttons */
#define CRUSHER_BUTTON_GPIO         4
#define MIXER_BUTTON_GPIO           23
#define FAN_BUTTON_GPIO             15

/* Actuators*/
#define CRUSHER_GPIO                33
#define MIXER_GPIO                  25
#define FAN_GPIO                    32
#define LOCK_GPIO                   26
// #define CRUSHER_GPIO_PROTOTYPE      26
// #define LOCK_PROTOTIPO_PROTOTYPE    33

/* Display */
#define I2C_PCF8574_ADDRESS         0x27
#define I2C_MASTER_GPIO_SCL         22
#define I2C_MASTER_GPIO_SDA         21

/* Humidity Sensor */
#define HUMIDITY_SENSOR_GPIO        27

/* Temperature Sensor */
#define TEMPERATURE_SENSOR_GPIO     5

/* Capacity Sensor */
#define HC_SR04_TRIG_GPIO           19
#define HC_SR04_ECHO_GPIO           18

/* Lid sensor */
#define LID_SENSOR_GPIO             34

#endif // GPIOS_H
