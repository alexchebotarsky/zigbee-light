#ifndef ZIGBEE_STACK_HPP
#define ZIGBEE_STACK_HPP

#include <functional>
#include <unordered_map>

#include "esp_zigbee_core.h"

using ActionKey = uint64_t;
using ActionHandler = std::function<esp_err_t(const void*)>;

template <typename T>
struct ActionHandlerMsg;

template <typename MsgType>
struct ActionHandlerMsg<esp_err_t (*)(const MsgType*)> {
  using type = MsgType;
};

template <typename T>
struct ActionHandlerMsg {
  using type = typename ActionHandlerMsg<decltype(&T::operator())>::type;
};

template <typename C, typename MsgType>
struct ActionHandlerMsg<esp_err_t (C::*)(const MsgType*) const> {
  using type = MsgType;
};

struct ActionCommonMessage {
  esp_zb_device_cb_common_info_t info;
};

class ZigbeeStack {
 public:
  esp_err_t init();
  esp_err_t start();
  esp_err_t register_endpoint(esp_zb_endpoint_config_t config,
                              esp_zb_cluster_list_t* clusters);

  template <typename Handler>
  void handle_action(esp_zb_core_action_callback_id_t callback_id,
                     uint8_t endpoint_id, uint16_t cluster_id,
                     Handler handler) {
    ActionKey key = make_action_key(callback_id, endpoint_id, cluster_id);
    ActionHandler action_handler = make_action_handler(handler);
    action_handlers.insert_or_assign(key, action_handler);
  }

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
  static ActionKey make_action_key(esp_zb_core_action_callback_id_t callback_id,
                                   uint8_t endpoint_id, uint16_t cluster_id);
  template <typename Handler>
  static ActionHandler make_action_handler(Handler handler) {
    using MsgType = typename ActionHandlerMsg<Handler>::type;
    return [handler](const void* msg) {
      return handler(static_cast<const MsgType*>(msg));
    };
  }

  esp_zb_ep_list_t* endpoints;
  std::unordered_map<ActionKey, ActionHandler> action_handlers;
};

extern ZigbeeStack& Zigbee;

#endif
