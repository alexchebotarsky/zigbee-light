#include <cstdio>

#include "LightDevice.hpp"
#include "SingleLED.hpp"
#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"
#include "nvs_flash.h"

constexpr int INBUILT_LED_PIN = 8;
SingleLED led(INBUILT_LED_PIN);

constexpr uint8_t LIGHT_ENDPOINT_ID = 10;
constexpr char MANUFACTURER_NAME[] = "Alex Chebotarsky";
constexpr char MODEL_IDENTIFIER[] = "Zigbee Light Device";

LightDevice light_device(DeviceConfig{
    .endpoint = LIGHT_ENDPOINT_ID,
    .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
    .app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID,
    .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
    .manufacturer = MANUFACTURER_NAME,
    .model = MODEL_IDENTIFIER,
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

  err = light_device.init();
  if (err != ESP_OK) {
    printf("Error initializing ZigbeeDevice: %s\n", esp_err_to_name(err));
    return;
  }

  err = Zigbee.register_endpoint(light_device.get_endpoint_config(),
                                 light_device.get_clusters());
  if (err != ESP_OK) {
    printf("Error registering light endpoint: %s\n", esp_err_to_name(err));
    return;
  }

  light_device.handle_action<esp_zb_zcl_set_attr_value_message_t>(
      ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID, [](const auto* msg) {
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
            break;
          }
          default:
            printf("Unsupported attribute ID: %u\n", msg->attribute.id);
            return ESP_ERR_NOT_SUPPORTED;
        }
      });

  err = Zigbee.start();
  if (err != ESP_OK) {
    printf("Error starting Zigbee: %s\n", esp_err_to_name(err));
    return;
  }
}
