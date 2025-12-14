#ifndef SINGLE_LED_HPP
#define SINGLE_LED_HPP

#include <cstdint>

#include "driver/gpio.h"
#include "led_strip.h"

class SingleLED {
 public:
  SingleLED(const int gpio_pin);
  esp_err_t init();

  esp_err_t set_color(uint32_t r, uint32_t g, uint32_t b);
  esp_err_t set_brightness(float value);
  esp_err_t transition_color(uint32_t r, uint32_t g, uint32_t b,
                             uint32_t duration_ms);
  esp_err_t transition_brightness(float value, uint32_t duration_ms);

 private:
  esp_err_t refresh();

  const gpio_num_t gpio;
  led_strip_handle_t led;
  bool is_initialized;
  uint32_t red;
  uint32_t green;
  uint32_t blue;
  float brightness;
};

#endif
