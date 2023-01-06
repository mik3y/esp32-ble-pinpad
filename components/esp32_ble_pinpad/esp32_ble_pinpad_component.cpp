#include "esp32_ble_pinpad_component.h"

#include "esphome/components/esp32_ble/ble.h"
#include "esphome/components/esp32_ble_server/ble_2902.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace esp32_ble_pinpad {

ESP32BLEPinpadComponent::ESP32BLEPinpadComponent() { }

void ESP32BLEPinpadComponent::set_static_secret_pin(const std::string &pin) {
  this->static_secret_pin_ = pin;
}

void ESP32BLEPinpadComponent::setup() {
  ESP_LOGD(TAG, "Setup starting ...");
  this->service_ = global_ble_server->create_service(SERVICE_UUID, true);
  this->setup_characteristics();
  ESP_LOGD(TAG, "Setup complete!");

  // TODO(mikey): Appropriate for us to start ourselves?
  this->start();
}

void ESP32BLEPinpadComponent::setup_characteristics() {
  ESP_LOGD(TAG, "Setting up characteristics ...");
  this->status_ = this->service_->create_characteristic(
      STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor *status_descriptor = new BLE2902();
  this->status_->add_descriptor(status_descriptor);

  this->rpc_ = this->service_->create_characteristic(RPC_COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  this->rpc_->on_write([this](const std::vector<uint8_t> &data) {
    if (!data.empty()) {
      this->incoming_data_.insert(this->incoming_data_.end(), data.begin(), data.end());
    }
  });
  BLEDescriptor *rpc_descriptor = new BLE2902();
  this->rpc_->add_descriptor(rpc_descriptor);

  this->rpc_response_ = this->service_->create_characteristic(
      RPC_RESPONSE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor *rpc_response_descriptor = new BLE2902();
  this->rpc_response_->add_descriptor(rpc_response_descriptor);

  this->setup_complete_ = true;
}

void ESP32BLEPinpadComponent::loop() {
  if (!this->incoming_data_.empty())
    this->process_incoming_data_();
  uint32_t now = millis();

  switch (this->state_) {
    case STATE_STOPPED:
      if (this->status_indicator_ != nullptr)
        this->status_indicator_->turn_off();

      if (this->service_->is_created() && this->should_start_ && this->setup_complete_) {
        if (this->service_->is_running()) {
          esp32_ble::global_ble->get_advertising()->start();

          this->set_state_(STATE_IDLE);
          this->should_start_ = false;
          ESP_LOGD(TAG, "Service started!");
        } else {
          this->service_->start();
        }
      }
      break;
    case STATE_IDLE : {
      if (this->status_indicator_ != nullptr)
        this->status_indicator_->turn_off();
      break;
    }
    case STATE_PIN_ACCEPTED : {
      this->incoming_data_.clear();
      break;
    }
    case STATE_PIN_REJECTED: {
      this->incoming_data_.clear();
      break;
    }
  }
}

void ESP32BLEPinpadComponent::set_state_(State state) {
  State old_state = this->state_;
  if (state == old_state) {
    return;
  }
  ESP_LOGV(TAG, "Setting state: %d", state);
  this->state_ = state;
  if (this->status_->get_value().empty() || this->status_->get_value()[0] != state) {
    uint8_t data[1]{state};
    this->status_->set_value(data, 1);
    if (old_state != STATE_STOPPED)
      this->status_->notify();
  }
}

void ESP32BLEPinpadComponent::send_response_(std::vector<uint8_t> &response) {
  this->rpc_response_->set_value(response);
  if (this->state_ != STATE_STOPPED)
    this->rpc_response_->notify();
}

void ESP32BLEPinpadComponent::start() {
  ESP_LOGD(TAG, "start() called");
  if (this->state_ != STATE_STOPPED)
    return;

  ESP_LOGD(TAG, "Setting BLE Pinpad to start");
  this->should_start_ = true;
}

void ESP32BLEPinpadComponent::stop() {
  ESP_LOGD(TAG, "stop() called");
  this->set_timeout("end-service", 1000, [this] {
    this->service_->stop();
    this->set_state_(STATE_STOPPED);
  });
}

float ESP32BLEPinpadComponent::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void ESP32BLEPinpadComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP32 BLE Pinpad:");
  ESP_LOGCONFIG(TAG, "  Status Indicator: '%s'", YESNO(this->status_indicator_ != nullptr));
}

void ESP32BLEPinpadComponent::process_incoming_data_() {
  size_t data_len = this->incoming_data_.size();
  if (data_len == 0) {
    return;
  }

  ESP_LOGD(TAG, "Processing bytes - %s", format_hex_pretty(this->incoming_data_).c_str());
  if (data_len > INPUT_MAX_LEN) {
    ESP_LOGV(TAG, "Too much data came in, or malformed resetting buffer...");
    this->incoming_data_.clear();
  } else {
    const char last_char = this->incoming_data_[data_len - 1];
    if (last_char == '\0' || last_char == '\n') {
      ESP_LOGV(TAG, "Processing pin input!");
      this->incoming_data_.clear();
    } else {
      ESP_LOGV(TAG, "Waiting for more data ...");
    }
  }
}

void ESP32BLEPinpadComponent::on_client_disconnect() { 
  this->incoming_data_.clear();
  this->set_state_(STATE_IDLE);
};

}  // namespace esp32_ble_pinpad
}  // namespace esphome

#endif
