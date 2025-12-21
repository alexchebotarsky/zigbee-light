#include "SingleLED.hpp"

#include <cmath>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

SingleLED::SingleLED(const int gpio_pin)
    : gpio(static_cast<gpio_num_t>(gpio_pin)),
      led(nullptr),
      red(0),
      green(0),
      blue(0),
      brightness(1) {}

esp_err_t SingleLED::init() {
  led_strip_config_t led_config = {
      .strip_gpio_num = gpio,
      .max_leds = 1,
      .led_model = LED_MODEL_WS2812,
      .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
      .flags = {
          .invert_out = false,
      }};

  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10 * 1000 * 1000,  // 10 MHz
      .mem_block_symbols = 64,
      .flags = {
          .with_dma = false,
      }};

  esp_err_t err = led_strip_new_rmt_device(&led_config, &rmt_config, &led);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t SingleLED::set_color(uint32_t r, uint32_t g, uint32_t b) {
  if (!is_valid_color(r, g, b)) {
    return ESP_ERR_INVALID_ARG;
  }

  red = r;
  green = g;
  blue = b;

  esp_err_t err = refresh();
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t SingleLED::set_brightness(float value) {
  // Validate input brightness value
  if (!is_valid_brightness(value)) {
    return ESP_ERR_INVALID_ARG;
  }

  brightness = value;

  esp_err_t err = refresh();
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t SingleLED::transition_color(uint32_t r, uint32_t g, uint32_t b,
                                      uint32_t duration_ms) {
  // Prevent division by zero
  if (duration_ms == 0) {
    return set_color(r, g, b);
  }

  // Validate input colors
  if (!is_valid_color(r, g, b)) {
    return ESP_ERR_INVALID_ARG;
  }

  const int steps = 100;

  // Duration must be divisible by steps in whole, otherwise it leads to uneven
  // transitions.
  if (duration_ms % steps != 0) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t err;

  float initial_r = red;
  float initial_g = green;
  float initial_b = blue;

  const int delay_ms = duration_ms / steps;

  for (int i = 0; i <= steps; i++) {
    float t = static_cast<float>(i) / steps;

    red = round(initial_r + ((r - initial_r) * t));
    green = round(initial_g + ((g - initial_g) * t));
    blue = round(initial_b + ((b - initial_b) * t));

    err = refresh();
    if (err != ESP_OK) {
      return err;
    }

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }

  return ESP_OK;
}

esp_err_t SingleLED::transition_brightness(float value, uint32_t duration_ms) {
  // Prevent division by zero
  if (duration_ms == 0) {
    return set_brightness(value);
  }

  // Validate input brightness value
  if (!is_valid_brightness(value)) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t err;
  float initial_brightness = brightness;

  const int steps = 100;
  const int delay_ms = duration_ms / steps;

  for (int i = 0; i <= steps; i++) {
    float t = static_cast<float>(i) / steps;

    brightness = initial_brightness + ((value - initial_brightness) * t);

    err = refresh();
    if (err != ESP_OK) {
      return err;
    }

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }

  return ESP_OK;
}

esp_err_t SingleLED::refresh() {
  uint32_t r = static_cast<uint32_t>(round(red * brightness));
  uint32_t g = static_cast<uint32_t>(round(green * brightness));
  uint32_t b = static_cast<uint32_t>(round(blue * brightness));

  esp_err_t err = led_strip_set_pixel(led, 0, r, g, b);
  if (err != ESP_OK) {
    return err;
  }

  err = led_strip_refresh(led);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

bool SingleLED::is_valid_color(uint32_t r, uint32_t g, uint32_t b) {
  return (r <= 255) && (g <= 255) && (b <= 255);
}

bool SingleLED::is_valid_brightness(float value) {
  return (value >= 0.0f) && (value <= 1.0f);
}
