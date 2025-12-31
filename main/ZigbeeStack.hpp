#ifndef ZIGBEE_STACK_HPP
#define ZIGBEE_STACK_HPP

#include <functional>
#include <unordered_map>

#include "esp_zigbee_core.h"

using ClusterKey = uint64_t;
using ClusterHandler =
    std::function<esp_err_t(esp_zb_core_action_callback_id_t, const void*)>;

struct ActionCommonMessage {
  esp_zb_device_cb_common_info_t info;
};

class ZigbeeStack {
 public:
  esp_err_t init();
  esp_err_t start();
  esp_err_t register_endpoint(esp_zb_endpoint_config_t& endpoint_config,
                              esp_zb_cluster_list_t* clusters);

  void handle_cluster(uint8_t endpoint_id, uint16_t cluster_id,
                      ClusterHandler handler);

  // Singleton instance accessor
  static ZigbeeStack& instance();

  // Prevent singleton copying
  ZigbeeStack(const ZigbeeStack&) = delete;
  ZigbeeStack& operator=(const ZigbeeStack&) = delete;

  // Prevent singleton moving
  ZigbeeStack(ZigbeeStack&&) = delete;
  ZigbeeStack& operator=(ZigbeeStack&&) = delete;

 private:
  // Private constructor for singleton pattern
  ZigbeeStack();

  static void task(void* pvParameters);
  static esp_err_t core_action_handler(
      esp_zb_core_action_callback_id_t callback_id, const void* message);
  static constexpr ClusterKey make_cluster_key(uint8_t endpoint_id,
                                               uint16_t cluster_id) {
    return (static_cast<uint64_t>(endpoint_id) << 16) |
           static_cast<uint64_t>(cluster_id);
  }

  esp_zb_ep_list_t* endpoints;
  std::unordered_map<ClusterKey, ClusterHandler> cluster_handlers;
};

extern ZigbeeStack& Zigbee;

#endif
