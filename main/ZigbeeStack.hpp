#ifndef ZIGBEE_STACK_HPP
#define ZIGBEE_STACK_HPP

#include <unordered_map>
#include <vector>

#include "esp_zigbee_core.h"

using AttributeKey = uint64_t;
using ActionCallback =
    esp_err_t (*)(const esp_zb_zcl_set_attr_value_message_t*);

class ZigbeeStack {
 public:
  esp_err_t init();
  esp_err_t start();
  void register_device(esp_zb_ep_list_t* device);
  void on_attribute_action(uint8_t endpoint_id, uint16_t cluster_id,
                           uint16_t attribute_id, ActionCallback callback);

  static void task(void* pvParameters);
  static esp_err_t action_handler(esp_zb_core_action_callback_id_t callback_id,
                                  const void* message);
  static AttributeKey make_attribute_key(uint8_t endpoint_id,
                                         uint16_t cluster_id,
                                         uint16_t attribute_id);

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

  std::vector<esp_zb_ep_list_t*> devices;
  std::unordered_map<AttributeKey, ActionCallback> attribute_handlers;
};

extern ZigbeeStack& Zigbee;

#endif
