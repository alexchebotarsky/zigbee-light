#include <cstdio>

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "sdkconfig.h"

constexpr const gpio_num_t LED_PIN = 8;

extern "C" void app_main(void) {
  led_strip_config_t led_config = {
      .strip_gpio_num = LED_PIN,
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

  /// Create the LED strip object
  led_strip_handle_t led;
  esp_err_t err = led_strip_new_rmt_device(&led_config, &rmt_config, &led);
  if (err != ESP_OK) {
    printf("Error creating new LED strip handle: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  while (true) {
    led_strip_set_pixel(led, 0, 255, 255, 255);
    led_strip_refresh(led);
    vTaskDelay(pdMS_TO_TICKS(1000));

    led_strip_set_pixel(led, 0, 0, 0, 0);
    led_strip_refresh(led);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
