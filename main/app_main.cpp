#include <cmath>
#include <cstdint>
#include <cstdio>

#include "SingleLED.hpp"
#include "TemperatureSensor.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

SingleLED led(CONFIG_INBUILT_LED_GPIO);

TemperatureSensor temperature_sensor(CONFIG_TEMPERATURE_SENSOR_SDA_GPIO,
                                     CONFIG_TEMPERATURE_SENSOR_SCL_GPIO);

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    printf("Error initializing NVS flash: %s\n", esp_err_to_name(err));
    return;
  }

  err = storage.init();
  if (err != ESP_OK) {
    printf("Error initializing Storage: %s\n", esp_err_to_name(err));
    return;
  }

  err = temperature_sensor.init();
  if (err != ESP_OK) {
    printf("Error initializing temperature sensor: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  while (true) {
    TemperatureReading reading = temperature_sensor.read();
    printf("Temperature: %.2f °C / Humidity: %.2f", reading.temperature,
           reading.humidity);
    led.set_color(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(2000));
    led.set_color(0, 0, 0);
  }
}
