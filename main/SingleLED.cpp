#include "SingleLED.hpp"

SingleLED::SingleLED(const int gpio_pin)
    : gpio(static_cast<gpio_num_t>(gpio_pin)), led(NULL), brightness(1) {}

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

esp_err_t SingleLED::refresh() {
  esp_err_t err = led_strip_set_pixel(led, 0, red * brightness,
                                      green * brightness, blue * brightness);
  if (err != ESP_OK) {
    return err;
  }

  err = led_strip_refresh(led);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t SingleLED::set_color(uint32_t r, uint32_t g, uint32_t b) {
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
  if (value > 1 || value < 0) {
    return ESP_ERR_INVALID_ARG;
  }

  brightness = value;

  esp_err_t err = refresh();
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}
