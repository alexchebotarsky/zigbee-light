#include "esp_check.h"
#include "esp_log.h"
#include "esp_zigbee_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "light_driver.h"
#include "nvs_flash.h"

const uint8_t LIGHT_ENDPOINT = 10;

static const char* TAG = "ESP_ZB_ON_OFF_LIGHT";

static esp_err_t deferred_driver_init(void) {
  static bool is_inited = false;
  if (!is_inited) {
    light_driver_init(LIGHT_DEFAULT_OFF);
    is_inited = true;
  }
  return is_inited ? ESP_OK : ESP_FAIL;
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask) {
  ESP_RETURN_ON_FALSE(
      esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG,
      "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct) {
  uint32_t* p_sg_p = signal_struct->p_app_signal;
  esp_err_t err_status = signal_struct->esp_err_status;
  esp_zb_app_signal_type_t sig_type = *p_sg_p;
  switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
      ESP_LOGI(TAG, "Initialize Zigbee stack");
      esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
      break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (err_status == ESP_OK) {
        ESP_LOGI(TAG, "Deferred driver initialization %s",
                 deferred_driver_init() ? "failed" : "successful");
        ESP_LOGI(TAG, "Device started up in%s factory-reset mode",
                 esp_zb_bdb_is_factory_new() ? "" : " non");
        if (esp_zb_bdb_is_factory_new()) {
          ESP_LOGI(TAG, "Start network steering");
          esp_zb_bdb_start_top_level_commissioning(
              ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
          ESP_LOGI(TAG, "Device rebooted");
        }
      } else {
        ESP_LOGW(TAG, "%s failed with status: %s, retrying",
                 esp_zb_zdo_signal_to_string(sig_type),
                 esp_err_to_name(err_status));
        esp_zb_scheduler_alarm(
            (esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
            ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
      }
      break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
      if (err_status == ESP_OK) {
        esp_zb_ieee_addr_t extended_pan_id;
        esp_zb_get_extended_pan_id(extended_pan_id);
        ESP_LOGI(TAG,
                 "Joined network successfully (Extended PAN ID: "
                 "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, "
                 "Channel:%d, Short Address: 0x%04hx)",
                 extended_pan_id[7], extended_pan_id[6], extended_pan_id[5],
                 extended_pan_id[4], extended_pan_id[3], extended_pan_id[2],
                 extended_pan_id[1], extended_pan_id[0], esp_zb_get_pan_id(),
                 esp_zb_get_current_channel(), esp_zb_get_short_address());
      } else {
        ESP_LOGI(TAG, "Network steering was not successful (status: %s)",
                 esp_err_to_name(err_status));
        esp_zb_scheduler_alarm(
            (esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
            ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
      }
      break;
    default:
      ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s",
               esp_zb_zdo_signal_to_string(sig_type), sig_type,
               esp_err_to_name(err_status));
      break;
  }
}

static esp_err_t zb_attribute_handler(
    const esp_zb_zcl_set_attr_value_message_t* message) {
  esp_err_t ret = ESP_OK;
  bool light_state = 0;

  ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
  ESP_RETURN_ON_FALSE(
      message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG,
      TAG, "Received message: error status(%d)", message->info.status);
  ESP_LOGI(TAG,
           "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), "
           "data size(%d)",
           message->info.dst_endpoint, message->info.cluster,
           message->attribute.id, message->attribute.data.size);
  if (message->info.dst_endpoint == LIGHT_ENDPOINT) {
    if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
      if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
          message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
        light_state = message->attribute.data.value
                          ? *(bool*)message->attribute.data.value
                          : light_state;
        ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
        light_driver_set_power(light_state);
      }
    }
  }
  return ret;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id,
                                   const void* message) {
  esp_err_t ret = ESP_OK;
  switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
      ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t*)message);
      break;
    default:
      ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
      break;
  }
  return ret;
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

  static char manufacturer_buf[32];
  static char model_buf[32];

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
  esp_zb_cfg_t zigbee_config = {
      .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
      .install_code_policy = false,
      .nwk_cfg.zed_cfg =
          {
              .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,
              .keep_alive = 3000,  // ms
          },
  };
  esp_zb_init(&zigbee_config);

  const uint8_t LIGHT_ENDPOINT = 10;

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

  esp_zb_core_action_handler_register(zb_action_handler);

  err = esp_zb_start(false);
  if (err != ESP_OK) {
    printf("Error starting Zigbee stack: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  esp_zb_stack_main_loop();
}

void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    printf("Error initializing NVS: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  esp_zb_platform_config_t platform_config = {
      .radio_config =
          {
              .radio_mode = ZB_RADIO_MODE_NATIVE,
          },
      .host_config =
          {
              .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
          },
  };
  err = esp_zb_platform_config(&platform_config);
  if (err != ESP_OK) {
    printf("Error configuring Zigbee platform: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
