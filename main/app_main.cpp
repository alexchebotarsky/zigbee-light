#include <cstdio>

#include "SingleLED.hpp"
#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"

constexpr const int INBUILT_LED_PIN = 8;
SingleLED led(INBUILT_LED_PIN);

constexpr uint8_t LIGHT_ENDPOINT = 10;

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    printf("Error initializing NVS: %s\n", esp_err_to_name(err));
    return;
  }

  err = led.init();
  if (err != ESP_OK) {
    printf("Error initializing LED: %s\n", esp_err_to_name(err));
    return;
  }

  err = Zigbee.init();
  if (err != ESP_OK) {
    printf("Error initializing Zigbee: %s\n", esp_err_to_name(err));
    return;
  }

  esp_zb_on_off_light_cfg_t light_config = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
  esp_zb_ep_list_t* light_device =
      esp_zb_on_off_light_ep_create(LIGHT_ENDPOINT, &light_config);
  Zigbee.register_device(light_device);

  Zigbee.on_attribute_action(
      LIGHT_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
      ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
      [](const esp_zb_zcl_set_attr_value_message_t* msg) {
        bool set_state = *static_cast<const bool*>(msg->attribute.data.value);
        printf("Setting light to %s\n", set_state ? "ON" : "OFF");
        if (set_state) {
          return led.transition_color(255, 255, 255, 1000);
        } else {
          return led.transition_color(0, 0, 0, 1000);
        }
      });

  err = Zigbee.start();
  if (err != ESP_OK) {
    printf("Error starting Zigbee: %s\n", esp_err_to_name(err));
    return;
  }
}
