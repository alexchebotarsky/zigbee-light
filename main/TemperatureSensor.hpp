#ifndef TEMPERATURE_SENSOR_HPP
#define TEMPERATURE_SENSOR_HPP

#include "driver/gpio.h"
#include "esp_err.h"
#include "sht4x.h"

struct TemperatureReading {
  float temperature;
  float humidity;
};

class TemperatureSensor {
 public:
  TemperatureSensor(const int sda_gpio_pin, const int scl_gpio_pin);
  esp_err_t init();

  TemperatureReading read();

 private:
  const gpio_num_t sda_gpio;
  const gpio_num_t scl_gpio;
  sht4x_t dev;
};

#endif
