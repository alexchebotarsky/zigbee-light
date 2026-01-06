#include "Storage.hpp"

#include "nvs_flash.h"

constexpr char NVS_NAMESPACE[] = "zigbee_device";

constexpr char ACTIVE_NVS_KEY[] = "active";
constexpr char BRIGHTNESS_NVS_KEY[] = "brightness";
constexpr char COLOR_X_NVS_KEY[] = "color_x";
constexpr char COLOR_Y_NVS_KEY[] = "color_y";

constexpr uint64_t DOUBLE_SCALE_FACTOR = (1ULL << 53);

Storage::Storage(bool default_active, double default_brightness,
                 double default_color_x, double default_color_y)
    : active(default_active),
      brightness(default_brightness),
      color_x(default_color_x),
      color_y(default_color_y) {}

esp_err_t Storage::init() {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) {
    return err;
  }

  uint8_t active_val;
  if (nvs_get_u8(nvs_storage, ACTIVE_NVS_KEY, &active_val) == ESP_OK) {
    this->active = active_val != 0;
  }

  uint64_t brightness_val;
  if (nvs_get_u64(nvs_storage, BRIGHTNESS_NVS_KEY, &brightness_val) == ESP_OK) {
    this->brightness = brightness_val / DOUBLE_SCALE_FACTOR;
  }

  uint64_t color_x_val;
  if (nvs_get_u64(nvs_storage, COLOR_X_NVS_KEY, &color_x_val) == ESP_OK) {
    this->color_x = color_x_val / DOUBLE_SCALE_FACTOR;
  }

  uint64_t color_y_val;
  if (nvs_get_u64(nvs_storage, COLOR_Y_NVS_KEY, &color_y_val) == ESP_OK) {
    this->color_y = color_y_val / DOUBLE_SCALE_FACTOR;
  }

  nvs_close(nvs_storage);
  return ESP_OK;
}

esp_err_t Storage::set_active(bool active) {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u8(nvs_storage, ACTIVE_NVS_KEY, active ? 1 : 0);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->active = active;

  nvs_close(nvs_storage);
  return ESP_OK;
}

bool Storage::get_active() { return active; }

esp_err_t Storage::set_brightness(double brightness) {
  if (brightness < 0.0 || brightness > 1.0) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u64(nvs_storage, BRIGHTNESS_NVS_KEY,
                    static_cast<uint64_t>(brightness * DOUBLE_SCALE_FACTOR));
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->brightness = brightness;

  nvs_close(nvs_storage);
  return ESP_OK;
}

double Storage::get_brightness() { return brightness; }

esp_err_t Storage::set_color_x(double color_x) {
  if (color_x < 0.0 || color_x > 1.0) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u64(nvs_storage, COLOR_X_NVS_KEY,
                    static_cast<uint64_t>(color_x * DOUBLE_SCALE_FACTOR));
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->color_x = color_x;

  nvs_close(nvs_storage);
  return ESP_OK;
}

double Storage::get_color_x() { return color_x; }

esp_err_t Storage::set_color_y(double color_y) {
  if (color_y < 0.0 || color_y > 1.0) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u64(nvs_storage, COLOR_Y_NVS_KEY,
                    static_cast<uint64_t>(color_y * DOUBLE_SCALE_FACTOR));
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->color_y = color_y;

  nvs_close(nvs_storage);
  return ESP_OK;
}

double Storage::get_color_y() { return color_y; }
