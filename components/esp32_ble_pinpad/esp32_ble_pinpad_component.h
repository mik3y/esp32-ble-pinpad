#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/esp32_ble_server/ble_characteristic.h"
#include "esphome/components/esp32_ble_server/ble_server.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"

#include <vector>

#ifdef USE_ESP32

namespace esphome {
namespace esp32_ble_pinpad {

using namespace esp32_ble_server;

static const char *const TAG = "esp32_ble_pinpad.component";

static const size_t INPUT_MAX_LEN = 255;

static const char *const SERVICE_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900000";
static const char *const STATUS_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900001";
static const char *const RPC_COMMAND_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900002";
static const char *const RPC_RESPONSE_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900003";

enum State : uint8_t {
  STATE_STOPPED = 0x00,
  STATE_IDLE = 0x01,
  STATE_PIN_ACCEPTED = 0x02,
  STATE_PIN_REJECTED = 0x03,
};

class ESP32BLEPinpadComponent : public Component, public BLEServiceComponent {
 public:
  ESP32BLEPinpadComponent();
  void dump_config() override;
  void loop() override;
  void setup() override;
  void setup_characteristics();
  void on_client_disconnect() override;

  float get_setup_priority() const override;
  void start() override;
  void stop() override;

  void set_static_secret_pin(const std::string &pin);

  bool is_active() const { return this->state_ != STATE_STOPPED; }

  void set_status_indicator(output::BinaryOutput *status_indicator) { this->status_indicator_ = status_indicator; }

 protected:
  bool should_start_{false};
  bool setup_complete_{false};

  State state_{STATE_STOPPED};

  std::vector<uint8_t> incoming_data_;
  std::string static_secret_pin_;

  std::shared_ptr<BLEService> service_;
  BLECharacteristic *status_;
  BLECharacteristic *rpc_;
  BLECharacteristic *rpc_response_;

  output::BinaryOutput *status_indicator_{nullptr};

  void set_state_(State state);
  void send_response_(std::vector<uint8_t> &response);
  void process_incoming_data_();
};

}  // namespace esp32_ble_pinpad
}  // namespace esphome

#endif
