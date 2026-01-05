#include <cstdint>
#include <cstdio>

#include "SingleLED.hpp"
#include "Storage.hpp"
#include "ZigbeeDevice.hpp"
#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"
#include "nvs_flash.h"

constexpr bool DEFAULT_ACTIVE = false;
constexpr uint8_t DEFAULT_LEVEL = 254;
constexpr uint16_t DEFAULT_X = 10000;
constexpr uint16_t DEFAULT_Y = 10000;
Storage storage(DEFAULT_ACTIVE, DEFAULT_LEVEL, DEFAULT_X, DEFAULT_Y);

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
      .on_off = storage.get_active(),
  };
  auto* on_off_attrs = esp_zb_on_off_cluster_create(&on_off_cfg);
  esp_err_t err = esp_zb_cluster_list_add_on_off_cluster(
      clusters, on_off_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  esp_zb_level_cluster_cfg_t level_cfg = {
      .current_level = storage.get_level(),
  };
  auto* level_attrs = esp_zb_level_cluster_create(&level_cfg);
  err = esp_zb_cluster_list_add_level_cluster(clusters, level_attrs,
                                              ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  esp_zb_color_cluster_cfg_t color_cfg = {
      .current_x = storage.get_color_x(),
      .current_y = storage.get_color_y(),
      .color_mode = ESP_ZB_ZCL_COLOR_CONTROL_COLOR_MODE_DEFAULT_VALUE,
      .options = ESP_ZB_ZCL_COLOR_CONTROL_OPTIONS_DEFAULT_VALUE,
      .enhanced_color_mode =
          ESP_ZB_ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_DEFAULT_VALUE,
      .color_capabilities = 0x0008,
  };
  auto* color_attrs = esp_zb_color_control_cluster_create(&color_cfg);
  err = esp_zb_cluster_list_add_color_control_cluster(
      clusters, color_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
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

  err = storage.init();
  if (err != ESP_OK) {
    printf("Error initializing Storage: %s\n", esp_err_to_name(err));
    return;
  }

  err = led.init(storage.get_active(),
                 static_cast<double>(storage.get_level()) / 254.0,
                 static_cast<double>(storage.get_color_x()) / 65535.0,
                 static_cast<double>(storage.get_color_y()) / 65535.0);
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
            esp_err_t err = storage.set_active(
                *static_cast<bool*>(msg->attribute.data.value));
            if (err != ESP_OK) return err;

            bool active = storage.get_active();
            printf("Setting active to '%s'\n", active ? "true" : "false");
            return led.set_active(active);
          }
          default:
            printf("Unsupported action: cluster_id=%u, attribute_id=%u\n",
                   msg->info.cluster, msg->attribute.id);
            return ESP_ERR_NOT_SUPPORTED;
        }
      });

  device.handle_action<esp_zb_zcl_set_attr_value_message_t>(
      ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID,
      [](const auto* msg) {
        switch (msg->attribute.id) {
          case ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID: {
            esp_err_t err = storage.set_level(
                *static_cast<uint8_t*>(msg->attribute.data.value));
            if (err != ESP_OK) return err;

            double level = static_cast<double>(storage.get_level()) / 254.0;
            printf("Setting level to %.2f\n", level);
            return led.set_level(level);
          }
          default:
            printf("Unsupported action: cluster_id=%u, attribute_id=%u\n",
                   msg->info.cluster, msg->attribute.id);
            return ESP_ERR_NOT_SUPPORTED;
        }
      });

  device.handle_action<esp_zb_zcl_set_attr_value_message_t>(
      ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID,
      [](const auto* msg) {
        switch (msg->attribute.id) {
          case ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID: {
            esp_err_t err = storage.set_color_x(
                *static_cast<uint16_t*>(msg->attribute.data.value));
            if (err != ESP_OK) return err;

            double x = static_cast<double>(storage.get_color_x()) / 65535.0;
            printf("Setting color X to %.2f\n", x);
            return led.set_color(x, led.get_color().y);
          }
          case ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID: {
            esp_err_t err = storage.set_color_y(
                *static_cast<uint16_t*>(msg->attribute.data.value));
            if (err != ESP_OK) return err;

            double y = static_cast<double>(storage.get_color_y()) / 65535.0;
            printf("Setting color Y to %.2f\n", y);
            return led.set_color(led.get_color().x, y);
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
