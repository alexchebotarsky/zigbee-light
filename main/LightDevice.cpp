#include "LightDevice.hpp"

LightDevice::LightDevice() {}

esp_err_t LightDevice::init() {
  clusters = esp_zb_zcl_cluster_list_create();

  // Basic cluster (required)
  esp_zb_basic_cluster_cfg_t basic_cfg = {
      .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
      .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
  };
  auto* basic_attrs = esp_zb_basic_cluster_create(&basic_cfg);
  esp_err_t err = esp_zb_cluster_list_add_basic_cluster(
      clusters, basic_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  // Identify cluster (required)
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

  return ESP_OK;
}

esp_zb_endpoint_config_t LightDevice::make_endpoint_config(
    uint8_t endpoint_id) {
  esp_zb_endpoint_config_t config = {};
  config.endpoint = endpoint_id;
  config.app_profile_id = ESP_ZB_AF_HA_PROFILE_ID;
  config.app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID;
  config.app_device_version = 1;

  return config;
}

esp_zb_cluster_list_t* LightDevice::get_clusters() { return clusters; }
