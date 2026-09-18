#include <cmath>
#include <cstdint>
#include <cstdio>

#include "SingleLED.hpp"
#include "Storage.hpp"
#include "TemperatureSensor.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

constexpr bool DEFAULT_ACTIVE = CONFIG_LIGHT_DEFAULT_ACTIVE != 0 ? true : false;
constexpr double DEFAULT_BRIGHTNESS = CONFIG_LIGHT_DEFAULT_BRIGHTNESS / 100.0;
constexpr double DEFAULT_COLOR_X = CONFIG_LIGHT_DEFAULT_COLOR_X / 100.0;
constexpr double DEFAULT_COLOR_Y = CONFIG_LIGHT_DEFAULT_COLOR_Y / 100.0;
Storage storage(DEFAULT_ACTIVE, DEFAULT_BRIGHTNESS, DEFAULT_COLOR_X,
                DEFAULT_COLOR_Y);

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

  err = led.init(storage.get_active(), storage.get_brightness(),
                 storage.get_color_x(), storage.get_color_y());
  if (err != ESP_OK) {
    printf("Error initializing SingleLED: %s\n", esp_err_to_name(err));
    return;
  }

  err = temperature_sensor.init();
  if (err != ESP_OK) {
    printf("Error initializing temperature sensor: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  while (true) {
    led.set_active(true);

    TemperatureReading reading = temperature_sensor.read();
    printf("Temperature: %.2f °C / Humidity: %.2f\n", reading.temperature,
           reading.humidity);

    led.set_active(false);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
