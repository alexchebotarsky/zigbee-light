#include "SingleLED.hpp"

#include <algorithm>
#include <cmath>

SingleLED::SingleLED(const int gpio_pin)
    : gpio(static_cast<gpio_num_t>(gpio_pin)),
      active(false),
      x(0.5),
      y(0.5),
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
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t SingleLED::set_color(double x, double y) {
  this->x = x;
  this->y = y;

  esp_err_t err = refresh();
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t SingleLED::set_brightness(double brightness) {
  if (brightness < 0.0 || brightness > 1.0) {
    return ESP_ERR_INVALID_ARG;
  }

  this->brightness = brightness;

  esp_err_t err = refresh();
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t SingleLED::set_active(bool active) {
  this->active = active;

  esp_err_t err = refresh();
  if (err != ESP_OK) return err;

  return ESP_OK;
}

ColorXY SingleLED::get_color() { return ColorXY{.x = x, .y = y}; }

double SingleLED::get_brightness() { return brightness; }

bool SingleLED::is_active() { return active; }

// PRIVATE METHODS
esp_err_t SingleLED::refresh() {
  ColorRGB rgb = get_color_rgb();

  printf("Updating LED: R=%d, G=%d, B=%d\n", rgb.r, rgb.g, rgb.b);

  esp_err_t err = led_strip_set_pixel(led, 0, rgb.r, rgb.g, rgb.b);
  if (err != ESP_OK) return err;

  err = led_strip_refresh(led);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

ColorRGB SingleLED::get_color_rgb() {
  if (y == 0) return ColorRGB{0, 0, 0};

  // Convert xy to XYZ color space
  double Y = 1.0;
  double X = (Y / y) * x;
  double Z = (Y / y) * (1.0 - x - y);

  // Convert XYZ to RGB using Wide RGB D65 conversion matrix
  double r = 3.2406 * X - 1.5372 * Y - 0.4986 * Z;
  double g = -0.9689 * X + 1.8758 * Y + 0.0415 * Z;
  double b = 0.0557 * X - 0.2040 * Y + 1.0570 * Z;

  // Normalize negative values
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  // Scale according to max value
  double maxc = std::max({r, g, b});
  if (maxc == 0) return ColorRGB{0, 0, 0};

  r /= maxc;
  g /= maxc;
  b /= maxc;

  // Apply brightness
  r *= brightness;
  g *= brightness;
  b *= brightness;

  // Convert to 8-bit RGB values
  ColorRGB rgb;
  rgb.r = static_cast<uint8_t>(std::round(r * 255.0));
  rgb.g = static_cast<uint8_t>(std::round(g * 255.0));
  rgb.b = static_cast<uint8_t>(std::round(b * 255.0));

  return rgb;
}
