#include <cstdio>

#include "LightDevice.hpp"
#include "SingleLED.hpp"
#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"
#include "nvs_flash.h"

constexpr const int INBUILT_LED_PIN = 8;
SingleLED led(INBUILT_LED_PIN);

LightDevice device(LightConfig{
    .endpoint_id = 10,
    .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
    .manufacturer = "Alex Chebotarsky",
    .model = "Zigbee Light Device",
});

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    printf("Error initializing NVS flash: %s\n", esp_err_to_name(err));
    return;
  }

  err = led.init();
  if (err != ESP_OK) {
    printf("Error initializing SingleLED: %s\n", esp_err_to_name(err));
    return;
  }

  err = Zigbee.init();
  if (err != ESP_OK) {
    printf("Error initializing ZigbeeStack: %s\n", esp_err_to_name(err));
    return;
  }

  err = device.init();
  if (err != ESP_OK) {
    printf("Error initializing LightDevice: %s\n", esp_err_to_name(err));
    return;
  }

  err = Zigbee.register_endpoint(device.get_endpoint_config(),
                                 device.get_clusters());
  if (err != ESP_OK) {
    printf("Error registering device endpoint: %s\n", esp_err_to_name(err));
    return;
  }

  Zigbee.handle_action(
      ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID, 10, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
      [](const esp_zb_zcl_set_attr_value_message_t* msg) {
        switch (msg->attribute.id) {
          case ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID: {
            bool set_state =
                *static_cast<const bool*>(msg->attribute.data.value);
            printf("Setting light to %s\n", set_state ? "ON" : "OFF");
            if (set_state) {
              return led.transition_color(255, 255, 255, 1000);
            } else {
              return led.transition_color(0, 0, 0, 1000);
            }
          }
          default:
            printf("Unsupported attribute type %d\n", msg->attribute.id);
            return ESP_ERR_NOT_SUPPORTED;
        }
      });

  err = Zigbee.start();
  if (err != ESP_OK) {
    printf("Error starting Zigbee: %s\n", esp_err_to_name(err));
    return;
  }
}
