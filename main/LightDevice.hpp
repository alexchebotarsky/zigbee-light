#ifndef LIGHT_DEVICE_HPP
#define LIGHT_DEVICE_HPP

#include "esp_zigbee_core.h"

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

 private:
  static void make_attr_string(const char* str, char* buf, size_t buf_len);

  const LightConfig config;
  esp_zb_cluster_list_t* clusters;
};

#endif
