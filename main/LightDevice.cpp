#include "LightDevice.hpp"

constexpr uint32_t APP_DEVICE_VERSION = 1;

// PUBLIC METHODS
LightDevice::LightDevice(const LightConfig config) : config(config) {}

esp_err_t LightDevice::init() {
  endpoint_config = esp_zb_endpoint_config_t{
      .endpoint = config.endpoint_id,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID,
      .app_device_version = APP_DEVICE_VERSION,
  };

  clusters = esp_zb_zcl_cluster_list_create();

  // Basic cluster
  esp_zb_basic_cluster_cfg_t basic_cfg = {
      .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
      .power_source = config.power_source,
  };
  auto* basic_attrs = esp_zb_basic_cluster_create(&basic_cfg);

  if (config.manufacturer != nullptr) {
    char manufacturer[32];
    make_attr_string(config.manufacturer, manufacturer, sizeof(manufacturer));
    esp_zb_basic_cluster_add_attr(
        basic_attrs, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer);
  }

  if (config.model != nullptr) {
    char model[32];
    make_attr_string(config.model, model, sizeof(model));
    esp_zb_basic_cluster_add_attr(
        basic_attrs, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model);
  }

  esp_err_t err = esp_zb_cluster_list_add_basic_cluster(
      clusters, basic_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  // Identify cluster
  esp_zb_identify_cluster_cfg_t identify_cfg = {
      .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
  };
  auto* identify_attrs = esp_zb_identify_cluster_create(&identify_cfg);
  err = esp_zb_cluster_list_add_identify_cluster(
      clusters, identify_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  // ON/OFF cluster
  esp_zb_on_off_cluster_cfg_t on_off_cfg = {
      .on_off = false,
  };
  auto* on_off_attrs = esp_zb_on_off_cluster_create(&on_off_cfg);
  err = esp_zb_cluster_list_add_on_off_cluster(clusters, on_off_attrs,
                                               ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }
  handle_cluster_actions(ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);

  return ESP_OK;
}

esp_zb_endpoint_config_t& LightDevice::get_endpoint_config() {
  return endpoint_config;
}

esp_zb_cluster_list_t* LightDevice::get_clusters() { return clusters; }

// PRIVATE METHODS
void LightDevice::make_attr_string(const char* str, char* buf, size_t buf_len) {
  if (str == nullptr) {
    buf[0] = 0;
    return;
  }

  size_t len = std::min(strlen(str), buf_len - 1);
  buf[0] = static_cast<uint8_t>(len);
  memcpy(&buf[1], str, len);
}

void LightDevice::handle_cluster_actions(uint16_t cluster_id) {
  Zigbee.handle_cluster(
      config.endpoint_id, cluster_id,
      [this](esp_zb_core_action_callback_id_t callback_id, const void* msg) {
        ActionKey key = make_action_key(callback_id);
        auto iter = this->action_handlers.find(key);
        if (iter != this->action_handlers.end()) {
          auto& [_, handler] = *iter;
          return handler(msg);
        }

        return ESP_OK;
      });
}
