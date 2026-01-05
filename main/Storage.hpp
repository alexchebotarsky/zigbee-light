#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <cstdint>

#include "esp_err.h"

class Storage {
 public:
  Storage(bool default_active, uint8_t default_level, uint16_t default_color_x,
          uint16_t default_color_y);
  esp_err_t init();

  esp_err_t set_active(bool active);
  bool get_active();

  esp_err_t set_level(uint8_t level);
  uint8_t get_level();

  esp_err_t set_color_x(uint16_t color_x);
  uint16_t get_color_x();

  esp_err_t set_color_y(uint16_t color_y);
  uint16_t get_color_y();

 private:
  bool active;
  uint8_t level;
  uint16_t color_x;
  uint16_t color_y;
};

#endif
