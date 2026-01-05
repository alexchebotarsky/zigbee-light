#include "Storage.hpp"

#include "nvs_flash.h"

constexpr char NVS_NAMESPACE[] = "zigbee_device";

constexpr char ACTIVE_NVS_KEY[] = "active";
constexpr char LEVEL_NVS_KEY[] = "level";
constexpr char COLOR_X_NVS_KEY[] = "color_x";
constexpr char COLOR_Y_NVS_KEY[] = "color_y";

Storage::Storage(bool default_active, uint8_t default_level,
                 uint16_t default_color_x, uint16_t default_color_y)
    : active(default_active),
      level(default_level),
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

  uint8_t level;
  if (nvs_get_u8(nvs_storage, LEVEL_NVS_KEY, &level) == ESP_OK) {
    this->level = level;
  }

  uint16_t color_x;
  if (nvs_get_u16(nvs_storage, COLOR_X_NVS_KEY, &color_x) == ESP_OK) {
    this->color_x = color_x;
  }

  uint16_t color_y;
  if (nvs_get_u16(nvs_storage, COLOR_Y_NVS_KEY, &color_y) == ESP_OK) {
    this->color_y = color_y;
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

esp_err_t Storage::set_level(uint8_t level) {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u8(nvs_storage, LEVEL_NVS_KEY, level);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->level = level;

  nvs_close(nvs_storage);
  return ESP_OK;
}

uint8_t Storage::get_level() { return level; }

esp_err_t Storage::set_color_x(uint16_t color_x) {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u16(nvs_storage, COLOR_X_NVS_KEY, color_x);
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

uint16_t Storage::get_color_x() { return color_x; }

esp_err_t Storage::set_color_y(uint16_t color_y) {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) return err;

  err = nvs_set_u16(nvs_storage, COLOR_Y_NVS_KEY, color_y);
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

uint16_t Storage::get_color_y() { return color_y; }
