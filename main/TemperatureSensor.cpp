#include "TemperatureSensor.hpp"

#include <cstdint>

#include "i2cdev.h"

TemperatureSensor::TemperatureSensor(const int sda_gpio_pin,
                                     const int scl_gpio_pin)
    : sda_gpio(static_cast<gpio_num_t>(sda_gpio_pin)),
      scl_gpio(static_cast<gpio_num_t>(scl_gpio_pin)),
      dev({}) {}

esp_err_t TemperatureSensor::init() {
  esp_err_t err = i2cdev_init();
  if (err != ESP_OK) {
    return err;
  }

  err = sht4x_init_desc(&dev, I2C_NUM_0, sda_gpio, scl_gpio);
  if (err != ESP_OK) {
    return err;
  }

  err = sht4x_init(&dev);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

TemperatureReading TemperatureSensor::read() {
  float temperature;
  float humidity;

  esp_err_t err = sht4x_measure(&dev, &temperature, &humidity);
  if (err != ESP_OK) {
    printf("Error reading from TemperatureSensor on SDA %d and SCL %d: %s\n",
           sda_gpio, scl_gpio, esp_err_to_name(err));
    return TemperatureReading{0, 0};
  }

  return TemperatureReading{temperature, humidity};
}
