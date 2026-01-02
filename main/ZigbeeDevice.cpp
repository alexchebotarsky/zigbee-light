#include "ZigbeeDevice.hpp"

constexpr uint32_t APP_DEVICE_VERSION = 1;

// PUBLIC METHODS
ZigbeeDevice::ZigbeeDevice(const DeviceConfig config) : config(config) {}

esp_err_t ZigbeeDevice::init(std::function<esp_err_t()> init_clusters) {
  clusters = esp_zb_zcl_cluster_list_create();

  esp_err_t err = init_basic_cluster();
  if (err != ESP_OK) {
    return err;
  }

  err = init_identify_cluster();
  if (err != ESP_OK) {
    return err;
  }

  err = init_clusters();
  if (err != ESP_OK) {
    return err;
  }

  esp_zb_endpoint_config_t endpoint_config = esp_zb_endpoint_config_t{
      .endpoint = config.endpoint,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .app_device_id = config.app_device_id,
      .app_device_version = APP_DEVICE_VERSION,
  };
  EndpointHandler handler = [this](uint16_t cluster_id, uint32_t callback_id,
                                   const void* msg) {
    ActionKey key = make_action_key(cluster_id, callback_id);
    auto iter = this->action_handlers.find(key);
    if (iter != this->action_handlers.end()) {
      auto& [_, handler] = *iter;
      return handler(msg);
    }
    return ESP_OK;
  };
  err = Zigbee.register_endpoint(endpoint_config, clusters, handler);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_zb_cluster_list_t* ZigbeeDevice::get_clusters() { return clusters; }

// PRIVATE METHODS
void ZigbeeDevice::make_attr_str(const char* str, char* buf, size_t buf_len) {
  if (str == nullptr) {
    buf[0] = 0;
    return;
  }

  size_t len = std::min(strlen(str), buf_len - 1);
  buf[0] = static_cast<uint8_t>(len);
  memcpy(&buf[1], str, len);
}

esp_err_t ZigbeeDevice::init_basic_cluster() {
  esp_zb_basic_cluster_cfg_t basic_cfg = {
      .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
      .power_source = config.power_source,
  };
  auto* basic_attrs = esp_zb_basic_cluster_create(&basic_cfg);

  if (config.manufacturer != nullptr) {
    char manufacturer[32];
    make_attr_str(config.manufacturer, manufacturer, sizeof(manufacturer));
    esp_zb_basic_cluster_add_attr(
        basic_attrs, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer);
  }

  if (config.model != nullptr) {
    char model[32];
    make_attr_str(config.model, model, sizeof(model));
    esp_zb_basic_cluster_add_attr(
        basic_attrs, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model);
  }

  esp_err_t err = esp_zb_cluster_list_add_basic_cluster(
      clusters, basic_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t ZigbeeDevice::init_identify_cluster() {
  esp_zb_identify_cluster_cfg_t identify_cfg = {
      .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
  };
  auto* identify_attrs = esp_zb_identify_cluster_create(&identify_cfg);

  esp_err_t err = esp_zb_cluster_list_add_identify_cluster(
      clusters, identify_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}
