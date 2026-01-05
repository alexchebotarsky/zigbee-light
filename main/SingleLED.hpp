#ifndef SINGLE_LED_HPP
#define SINGLE_LED_HPP

#include <cstdint>

#include "driver/gpio.h"
#include "led_strip.h"

struct ColorRGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct ColorXY {
  double x;
  double y;
};

class SingleLED {
 public:
  SingleLED(const int gpio_pin);
  esp_err_t init();

  esp_err_t set_color(double x, double y);
  esp_err_t set_brightness(double value);
  esp_err_t set_active(bool active);

  ColorXY get_color();
  double get_brightness();
  bool is_active();

 private:
  esp_err_t refresh();

  ColorRGB get_color_rgb();

  const gpio_num_t gpio;
  led_strip_handle_t led;
  bool active;
  double x;
  double y;
  double brightness;
};

#endif
