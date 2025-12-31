#ifndef LIGHT_DEVICE_HPP
#define LIGHT_DEVICE_HPP

#include "ZigbeeDevice.hpp"
#include "esp_zigbee_core.h"

class LightDevice : public ZigbeeDevice {
 public:
  using ZigbeeDevice::ZigbeeDevice;

 protected:
  esp_err_t init_clusters() override;
};

#endif
