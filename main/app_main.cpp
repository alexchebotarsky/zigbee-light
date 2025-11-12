#include <cstdio>

#include "SingleLED.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

constexpr const int INBUILT_LED_PIN = 8;

SingleLED led(INBUILT_LED_PIN);

extern "C" void app_main(void) {
  esp_err_t err = led.init();
  if (err != ESP_OK) {
    printf("error initializing led: %s", esp_err_to_name(err));
    esp_restart();
  }

  led.set_brightness(0.05);

  while (true) {
    printf("Transitioning LED to red\n");
    led.transition_color(255, 0, 0, 1000);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Transitioning LED to green\n");
    led.transition_color(0, 255, 0, 1000);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Transitioning LED to blue\n");
    led.transition_color(0, 0, 255, 1000);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
