# zigbee-light

This is a simple Zigbee light device implementation. It's built with
[ESP-IDF](https://github.com/espressif/esp-idf) and designed for ESP32-H2
MCU boards.

This implementation provides 2 reusable C++ abstractions:

- `ZigbeeStack`

  Top-level Zigbee initialization and FreeRTOS task handling.
  
  It exposes a method for registering endpoints provided endpoint configuration
  and event handler.

- `ZigbeeDevice`

  A device registers a single endpoint with `ZigbeeStack` and can facilitate
  multiple clusters for that endpoint. Multiple endpoints can be achieved by
  making multiple devices. Device initializes required common clusters by
  default, and custom device-specific clusters can be setup using a callback.

  It exposes a method for handling actions provided cluster ID and callback ID.
  The method is generic, so it is possible to specify a type for the message
  that callback will receive (it's based on the callback ID).
