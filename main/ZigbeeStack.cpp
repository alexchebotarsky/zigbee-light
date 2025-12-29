#include "ZigbeeStack.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

constexpr const char* TASK_NAME = "ZigbeeStack";
constexpr int TASK_STACK_SIZE = 4096;
constexpr int TASK_PRIORITY = 5;

// Globally accessible singleton instance
ZigbeeStack& Zigbee = ZigbeeStack::instance();

// Private singleton constructor
ZigbeeStack::ZigbeeStack()
    : endpoints(esp_zb_ep_list_create()), action_handlers() {}

// Singleton instance accessor
ZigbeeStack& ZigbeeStack::instance() {
  static ZigbeeStack instance;
  return instance;
}

// PUBLIC METHODS
esp_err_t ZigbeeStack::init() {
  esp_zb_platform_config_t platform_config = {
      .radio_config =
          {
              .radio_mode = ZB_RADIO_MODE_NATIVE,
              .radio_uart_config = {},
          },
      .host_config =
          {
              .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
              .host_uart_config = {},
          },
  };
  esp_err_t err = esp_zb_platform_config(&platform_config);
  if (err != ESP_OK) {
    return err;
  }

  esp_zb_core_action_handler_register(core_action_handler);

  return ESP_OK;
}

esp_err_t ZigbeeStack::start() {
  BaseType_t result = xTaskCreate(task, TASK_NAME, TASK_STACK_SIZE, nullptr,
                                  TASK_PRIORITY, nullptr);
  return (result == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_err_t ZigbeeStack::register_endpoint(esp_zb_endpoint_config_t config,
                                         esp_zb_cluster_list_t* clusters) {
  esp_err_t err = esp_zb_ep_list_add_ep(endpoints, clusters, config);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

// PRIVATE METHODS
void ZigbeeStack::task(void* pvParameters) {
  esp_zb_cfg_t zigbee_cfg = {
      .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
      .install_code_policy = false,
      .nwk_cfg =
          {
              .zed_cfg =
                  {
                      .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,
                      .keep_alive = 3000,
                  },
          },
  };
  esp_zb_init(&zigbee_cfg);

  esp_err_t err = esp_zb_device_register(Zigbee.endpoints);
  if (err != ESP_OK) {
    printf("Error registering Zigbee endpoints: %s\n", esp_err_to_name(err));
    return;
  }

  err = esp_zb_start(false);
  if (err != ESP_OK) {
    printf("Error starting Zigbee stack: %s\n", esp_err_to_name(err));
    return;
  }

  esp_zb_stack_main_loop();
}

esp_err_t ZigbeeStack::core_action_handler(
    esp_zb_core_action_callback_id_t callback_id, const void* msg) {
  const auto* common = static_cast<const ActionCommonMessage*>(msg);

  ActionKey key = make_action_key(callback_id, common->info.dst_endpoint,
                                  common->info.cluster);

  const auto iter = Zigbee.action_handlers.find(key);
  if (iter != Zigbee.action_handlers.end()) {
    auto& [_, handler] = *iter;
    return handler(msg);
  }

  return ESP_ERR_NOT_SUPPORTED;
}

ActionKey ZigbeeStack::make_action_key(
    esp_zb_core_action_callback_id_t callback_id, uint8_t endpoint_id,
    uint16_t cluster_id) {
  return (static_cast<uint64_t>(callback_id) << 24) |
         (static_cast<uint64_t>(endpoint_id) << 16) |
         static_cast<uint64_t>(cluster_id);
}

// GLOBAL ZIGBEE SIGNAL HANDLER
extern "C" void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct) {
  esp_zb_app_signal_type_t sig_type =
      static_cast<esp_zb_app_signal_type_t>(*signal_struct->p_app_signal);
  esp_err_t err = signal_struct->esp_err_status;

  esp_zb_callback_t start_commissioning = [](uint8_t mode_mask) {
    esp_err_t err = esp_zb_bdb_start_top_level_commissioning(mode_mask);
    if (err != ESP_OK) {
      printf("Error starting top level commissioning: %s\n",
             esp_err_to_name(err));
    }
  };

  switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
      printf("Initializing Zigbee stack\n");
      start_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
      break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (err != ESP_OK) {
        printf("Restarting Zigbee stack: %s\n", esp_err_to_name(err));
        esp_zb_scheduler_alarm(start_commissioning,
                               ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
        break;
      }
      if (esp_zb_bdb_is_factory_new()) {
        printf("Starting network steering for factory new device\n");
        start_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
      }
      break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
      if (err != ESP_OK) {
        printf("Restarting network steering: %s\n", esp_err_to_name(err));
        esp_zb_scheduler_alarm(start_commissioning,
                               ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        break;
      }
      printf("Joined network successfully!\n");
      break;
    default:
      printf("Unhandled Zigbee signal %s: %s\n",
             esp_zb_zdo_signal_to_string(sig_type), esp_err_to_name(err));
      break;
  }
}
