#include "LightDevice.hpp"

esp_err_t LightDevice::init_clusters() {
  esp_zb_on_off_cluster_cfg_t on_off_cfg = {
      .on_off = false,
  };
  auto* on_off_attrs = esp_zb_on_off_cluster_create(&on_off_cfg);

  esp_err_t err = esp_zb_cluster_list_add_on_off_cluster(
      get_clusters(), on_off_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (err != ESP_OK) {
    return err;
  }

  register_cluster(ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);

  return ESP_OK;
}
