#include <cstdint>
#include <cstdio>

#include "SingleLED.hpp"
#include "ZigbeeDevice.hpp"
#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"
#include "nvs_flash.h"

constexpr int LED_PIN = 8;
SingleLED led(LED_PIN);

constexpr uint8_t LIGHT_ENDPOINT_ID = 10;
constexpr char MANUFACTURER_NAME[] = "Alex Chebotarsky";
constexpr char MODEL_IDENTIFIER[] = "Zigbee Light Device";
ZigbeeDevice device(DeviceConfig{
    .endpoint = LIGHT_ENDPOINT_ID,
    .app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID,
    .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
    .manufacturer = MANUFACTURER_NAME,
    .model = MODEL_IDENTIFIER,
});

esp_err_t setup_clusters(esp_zb_cluster_list_t* clusters) {
  esp_zb_on_off_cluster_cfg_t on_off_cfg = {
      .on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE,
  };
  auto* on_off_attrs = esp_zb_on_off_cluster_create(&on_off_cfg);
  esp_err_t err = esp_zb_cluster_list_add_on_off_cluster(
      clusters, on_off_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

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

  err = device.init(setup_clusters);
  if (err != ESP_OK) {
    printf("Error initializing ZigbeeDevice: %s\n", esp_err_to_name(err));
    return;
  }

  device.handle_action<esp_zb_zcl_set_attr_value_message_t>(
      ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID,
      [](const auto* msg) {
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
            printf("Unsupported action: cluster_id=%u, attribute_id=%u\n",
                   msg->info.cluster, msg->attribute.id);
            return ESP_ERR_NOT_SUPPORTED;
        }
      });

  err = Zigbee.start();
  if (err != ESP_OK) {
    printf("Error starting Zigbee: %s\n", esp_err_to_name(err));
    return;
  }
}
