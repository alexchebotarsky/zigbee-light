#include <cstdio>
#include <cstring>

#include "SingleLED.hpp"
#include "esp_zigbee_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"

constexpr uint8_t LIGHT_ENDPOINT = 10;
constexpr const int INBUILT_LED_PIN = 8;

SingleLED led(INBUILT_LED_PIN);

static void start_top_level_commissioning(uint8_t mode_mask) {
  esp_err_t err = esp_zb_bdb_start_top_level_commissioning(mode_mask);
  if (err != ESP_OK) {
    printf("Error starting top level commissioning: %s\n",
           esp_err_to_name(err));
  }
}

extern "C" void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct) {
  esp_zb_app_signal_type_t sig_type =
      static_cast<esp_zb_app_signal_type_t>(*signal_struct->p_app_signal);
  esp_err_t err_status = signal_struct->esp_err_status;
  esp_err_t err;

  switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
      printf("Initializing Zigbee stack\n");
      start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
      break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (err_status != ESP_OK) {
        printf("Restarting Zigbee stack: %s\n", esp_err_to_name(err_status));
        esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning,
                               ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
        break;
      }

      err = led.init();
      if (err != ESP_OK) {
        printf("Error initializing LED: %s\n", esp_err_to_name(err));
        break;
      }

      if (esp_zb_bdb_is_factory_new()) {
        printf("Starting network steering for factory new device\n");
        start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
      }
      break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
      if (err_status != ESP_OK) {
        printf("Restarting network steering: %s\n",
               esp_err_to_name(err_status));
        esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning,
                               ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        break;
      }

      printf("Joined network successfully!\n");
      break;
    default:
      printf("Unhandled Zigbee signal %s: %s\n",
             esp_zb_zdo_signal_to_string(sig_type),
             esp_err_to_name(err_status));
      break;
  }
}

static esp_err_t handle_attribute(
    const esp_zb_zcl_set_attr_value_message_t* msg) {
  if (!msg) {
    return ESP_ERR_INVALID_ARG;
  }

  if (msg->info.status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    return ESP_ERR_INVALID_ARG;
  }

  printf("Message: endpoint(%d), cluster(0x%x), attribute(0x%x), size(%d)\n",
         msg->info.dst_endpoint, msg->info.cluster, msg->attribute.id,
         msg->attribute.data.size);

  if (msg->info.dst_endpoint == LIGHT_ENDPOINT) {
    if (msg->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
      if (msg->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
          msg->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
        bool light_state = msg->attribute.data.value
                               ? *(bool*)msg->attribute.data.value
                               : false;

        printf("Setting light to %s\n", light_state ? "ON" : "OFF");
        if (light_state) {
          return led.transition_color(255, 255, 255, 1000);
        } else {
          return led.transition_color(0, 0, 0, 1000);
        }
      }
    }
  }

  return ESP_OK;
}

static esp_err_t handle_action(esp_zb_core_action_callback_id_t callback_id,
                               const void* message) {
  switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
      return handle_attribute((esp_zb_zcl_set_attr_value_message_t*)message);
    default:
      return ESP_ERR_NOT_SUPPORTED;
  }
}

static esp_err_t set_basic_info(esp_zb_ep_list_t* ep_list, uint8_t endpoint_id,
                                const char* manufacturer, const char* model) {
  esp_zb_cluster_list_t* cluster_list =
      esp_zb_ep_list_get_ep(ep_list, endpoint_id);
  if (!cluster_list) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_zb_attribute_list_t* basic_cluster =
      esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC,
                                      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (!basic_cluster) {
    return ESP_ERR_INVALID_ARG;
  }

  char manufacturer_buf[32];
  char model_buf[32];

  uint8_t manufacturer_len = strlen(manufacturer);
  uint8_t model_len = strlen(model);

  if (manufacturer_len > sizeof(manufacturer_buf) - 1 ||
      model_len > sizeof(model_buf) - 1) {
    return ESP_ERR_INVALID_ARG;
  }

  manufacturer_buf[0] = manufacturer_len;
  memcpy(&manufacturer_buf[1], manufacturer, manufacturer_len);

  esp_err_t err = esp_zb_basic_cluster_add_attr(
      basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
      manufacturer_buf);
  if (err != ESP_OK) {
    return err;
  }

  model_buf[0] = model_len;
  memcpy(&model_buf[1], model, model_len);

  err = esp_zb_basic_cluster_add_attr(
      basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model_buf);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

static void esp_zb_task(void* pvParameters) {
  esp_zb_cfg_t zigbee_config = {};
  zigbee_config.esp_zb_role = ESP_ZB_DEVICE_TYPE_ED;
  zigbee_config.install_code_policy = false;
  zigbee_config.nwk_cfg.zed_cfg.ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN;
  zigbee_config.nwk_cfg.zed_cfg.keep_alive = 3000;
  esp_zb_init(&zigbee_config);

  esp_zb_on_off_light_cfg_t light_config = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
  esp_zb_ep_list_t* light_ep =
      esp_zb_on_off_light_ep_create(LIGHT_ENDPOINT, &light_config);

  const char* MANUFACTURER = "Alex Chebotarsky";
  const char* MODEL = "my_zigbee_device";

  esp_err_t err = set_basic_info(light_ep, LIGHT_ENDPOINT, MANUFACTURER, MODEL);
  if (err != ESP_OK) {
    printf("Error setting basic info: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  err = esp_zb_device_register(light_ep);
  if (err != ESP_OK) {
    printf("Error registering Zigbee device: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  esp_zb_core_action_handler_register(handle_action);

  err = esp_zb_start(false);
  if (err != ESP_OK) {
    printf("Error starting Zigbee stack: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  esp_zb_stack_main_loop();
}

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    printf("Error initializing NVS: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  esp_zb_platform_config_t platform_config = {};
  platform_config.radio_config.radio_mode = ZB_RADIO_MODE_NATIVE;
  platform_config.host_config.host_connection_mode =
      ZB_HOST_CONNECTION_MODE_NONE;
  err = esp_zb_platform_config(&platform_config);
  if (err != ESP_OK) {
    printf("Error configuring Zigbee platform: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
