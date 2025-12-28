#ifndef LIGHT_DEVICE_HPP
#define LIGHT_DEVICE_HPP

#include "esp_zigbee_core.h"

class LightDevice {
 public:
  LightDevice();
  esp_err_t init();
  esp_zb_endpoint_config_t make_endpoint_config(uint8_t endpoint_id);
  esp_zb_cluster_list_t* get_clusters();

 private:
  esp_zb_cluster_list_t* clusters;
};

#endif
