#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <cstdint>

#include "esp_err.h"

class Storage {
 public:
  Storage(bool default_active, double default_brightness,
          double default_color_x, double default_color_y);
  esp_err_t init();

  esp_err_t set_active(bool active);
  bool get_active();

  esp_err_t set_brightness(double brightness);
  double get_brightness();

  esp_err_t set_color_x(double color_x);
  double get_color_x();

  esp_err_t set_color_y(double color_y);
  double get_color_y();

 private:
  bool active;
  double brightness;
  double color_x;
  double color_y;
};

#endif
