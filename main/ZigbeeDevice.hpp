#ifndef ZIGBEE_DEVICE_HPP
#define ZIGBEE_DEVICE_HPP

#include <functional>
#include <unordered_map>

#include "ZigbeeStack.hpp"
#include "esp_zigbee_core.h"

using ActionKey = uint64_t;
using ActionHandler = std::function<esp_err_t(const void* msg)>;

struct DeviceConfig {
  uint8_t endpoint;
  esp_zb_zcl_basic_power_source_t power_source =
      ESP_ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
  const char* manufacturer = nullptr;
  const char* model = nullptr;
};

class ZigbeeDevice {
 public:
  ZigbeeDevice(const DeviceConfig config);
  esp_err_t init();

  template <typename T>
  void handle_action(uint16_t cluster_id, uint32_t callback_id,
                     std::function<esp_err_t(const T*)> handler) {
    ActionKey key = make_action_key(cluster_id, callback_id);
    ActionHandler action_handler = make_action_handler(handler);
    action_handlers.insert_or_assign(key, action_handler);
  }

 private:
  static void make_attr_str(const char* str, char* buf, size_t buf_len);
  static constexpr ActionKey make_action_key(uint16_t cluster_id,
                                             uint32_t callback_id) {
    return (static_cast<uint64_t>(callback_id) << 16) |
           static_cast<uint64_t>(cluster_id);
  }
  template <typename T>
  static ActionHandler make_action_handler(
      std::function<esp_err_t(const T*)> handler) {
    return [handler](const void* msg) {
      return handler(static_cast<const T*>(msg));
    };
  }

  esp_err_t init_basic_cluster();
  esp_err_t init_identify_cluster();
  esp_err_t init_main_clusters();

  esp_zb_cluster_list_t* clusters;
  std::unordered_map<ActionKey, ActionHandler> action_handlers;
  const DeviceConfig config;
};

#endif
