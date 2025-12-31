#ifndef LIGHT_DEVICE_HPP
#define LIGHT_DEVICE_HPP

#include <functional>
#include <unordered_map>

#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"

using AttributeKey = uint16_t;
using AttributeHandler =
    std::function<esp_err_t(const esp_zb_zcl_set_attr_value_message_t*)>;

struct LightConfig {
  uint8_t endpoint_id;
  esp_zb_zcl_basic_power_source_t power_source =
      ESP_ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
  const char* manufacturer = nullptr;
  const char* model = nullptr;
};

class LightDevice {
 public:
  LightDevice(const LightConfig config);
  esp_err_t init();
  esp_zb_endpoint_config_t get_endpoint_config();
  esp_zb_cluster_list_t* get_clusters();

  void handle_attribute(AttributeKey key, AttributeHandler handler) {
    attribute_handlers.insert_or_assign(key, handler);
  }

 private:
  static void make_attr_string(const char* str, char* buf, size_t buf_len);

  std::unordered_map<AttributeKey, AttributeHandler> attribute_handlers;

  const LightConfig config;
  esp_zb_endpoint_config_t endpoint_config;
  esp_zb_cluster_list_t* clusters;
};

#endif
