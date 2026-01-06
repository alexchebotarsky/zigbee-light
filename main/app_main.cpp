#include <cmath>
#include <cstdint>
#include <cstdio>

#include "SingleLED.hpp"
#include "Storage.hpp"
#include "ZigbeeDevice.hpp"
#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"
#include "nvs_flash.h"

constexpr bool DEFAULT_ACTIVE = CONFIG_LIGHT_DEFAULT_ACTIVE != 0 ? true : false;
constexpr double DEFAULT_BRIGHTNESS = CONFIG_LIGHT_DEFAULT_BRIGHTNESS / 100.0;
constexpr double DEFAULT_COLOR_X = CONFIG_LIGHT_DEFAULT_COLOR_X / 100.0;
constexpr double DEFAULT_COLOR_Y = CONFIG_LIGHT_DEFAULT_COLOR_Y / 100.0;
Storage storage(DEFAULT_ACTIVE, DEFAULT_BRIGHTNESS, DEFAULT_COLOR_X,
                DEFAULT_COLOR_Y);

SingleLED led(CONFIG_LED_PIN);

ZigbeeDevice device(DeviceConfig{
    .endpoint = CONFIG_LIGHT_ENDPOINT,
    .app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID,
    .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
    .manufacturer = CONFIG_DEVICE_MANUFACTURER,
    .model = CONFIG_DEVICE_MODEL,
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
      .current_level =
          static_cast<uint8_t>(std::round(storage.get_brightness() * 254.0)),
  };
  auto* level_attrs = esp_zb_level_cluster_create(&level_cfg);
  err = esp_zb_cluster_list_add_level_cluster(clusters, level_attrs,
                                              ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  esp_zb_color_cluster_cfg_t color_cfg = {
      .current_x =
          static_cast<uint16_t>(std::round(storage.get_color_x() * 65535.0)),
      .current_y =
          static_cast<uint16_t>(std::round(storage.get_color_y() * 65535.0)),
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

  err = led.init(storage.get_active(), storage.get_brightness(),
                 storage.get_color_x(), storage.get_color_y());
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
            bool active = *static_cast<bool*>(msg->attribute.data.value);

            esp_err_t err = storage.set_active(active);
            if (err != ESP_OK) return err;

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
            double brightness =
                *static_cast<uint8_t*>(msg->attribute.data.value) / 254.0;

            esp_err_t err = storage.set_brightness(brightness);
            if (err != ESP_OK) return err;

            return led.set_brightness(brightness);
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
            double x =
                *static_cast<uint16_t*>(msg->attribute.data.value) / 65535.0;

            esp_err_t err = storage.set_color_x(x);
            if (err != ESP_OK) return err;

            return led.set_color(x, led.get_color().y);
          }
          case ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID: {
            double y =
                *static_cast<uint16_t*>(msg->attribute.data.value) / 65535.0;

            esp_err_t err = storage.set_color_y(y);
            if (err != ESP_OK) return err;

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
