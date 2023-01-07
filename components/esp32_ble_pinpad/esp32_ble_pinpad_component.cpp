#include "esp32_ble_pinpad_component.h"
#include "otp.h"

#include "esphome/components/esp32_ble/ble.h"
#include "esphome/components/esp32_ble_server/ble_2902.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include <cassert>

#ifdef USE_ESP32

namespace esphome {
namespace esp32_ble_pinpad {

ESP32BLEPinpadComponent::ESP32BLEPinpadComponent() { }

void ESP32BLEPinpadComponent::set_security_mode(
    SecurityMode mode,
    const std::string &secret_passcode
) {
  ESP_LOGD(TAG, "Setting security mode ...");
  this->security_mode_ = mode;
  this->secret_passcode_ = secret_passcode;
}

void ESP32BLEPinpadComponent::setup() {
  ESP_LOGD(TAG, "Setup starting ...");
  this->service_ = global_ble_server->create_service(PINPAD_SERVICE_UUID, true);
  this->hotp_counter_ = global_preferences->make_preference<uint32_t>(0);
  this->setup_characteristics();
  ESP_LOGD(TAG, "Setup complete!");

  // TODO(mikey): Appropriate for us to start ourselves?
  this->start();
}

void ESP32BLEPinpadComponent::setup_characteristics() {
  ESP_LOGD(TAG, "Setting up characteristics ...");

  // Status characteristic. Contains the state of the device (idle, accepted, rejected, etc).
  this->status_ = this->service_->create_characteristic(
      PINPAD_STATUS_CHR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor *status_descriptor = new BLE2902();
  this->status_->add_descriptor(status_descriptor);

  // "RPC" characteristic. Where we'll receive the pin input.
  this->rpc_ = this->service_->create_characteristic(PINPAD_RPC_COMMAND_CHR_UUID, BLECharacteristic::PROPERTY_WRITE);
  this->rpc_->on_write([this](const std::vector<uint8_t> &data) {
    if (!data.empty()) {
      this->incoming_data_.insert(this->incoming_data_.end(), data.begin(), data.end());
    }
  });
  BLEDescriptor *rpc_descriptor = new BLE2902();
  this->rpc_->add_descriptor(rpc_descriptor);

  // Security mode characteristic. Tells the client what sort of pin we're expecting.
  this->security_mode_characteristic_ = this->service_->create_characteristic(
      PINPAD_SECURITY_MODE_CHR_UUID, BLECharacteristic::PROPERTY_READ);
  BLEDescriptor *security_mode_descriptor = new BLE2902();
  this->security_mode_characteristic_->add_descriptor(security_mode_descriptor);
  switch (this->security_mode_) {
    case SECURITY_MODE_NONE:
      this->security_mode_characteristic_->set_value(std::string("none"));
      break;
    case SECURITY_MODE_HOTP:
      this->security_mode_characteristic_->set_value(std::string("hotp"));
      break;
    case SECURITY_MODE_TOTP:
      this->security_mode_characteristic_->set_value(std::string("totp"));
      break;
    default:
      assert(false);
      break;
  }

  // HOTP counter characteristic. Only pertinent when security mode is hotp. Gives
  // our current (persisted) hotp counter value.
  this->hotp_counter_characteristic_ = this->service_->create_characteristic(
      PINPAD_HOTP_COUNTER_CHR_UUID, BLECharacteristic::PROPERTY_READ);
  BLEDescriptor *hotp_counter_descriptor = new BLE2902();
  this->hotp_counter_characteristic_->add_descriptor(hotp_counter_descriptor);
  this->hotp_counter_characteristic_->set_value(std::to_string(this->get_current_hotp_counter()));

  this->setup_complete_ = true;
}

uint32_t ESP32BLEPinpadComponent::get_current_hotp_counter() {
  uint32_t tmp;
  if (this->hotp_counter_.load(&tmp)) {
    return tmp;
  }
  ESP_LOGW(TAG, "Failed to fetch hotp counter");
  return 0;
}

uint32_t ESP32BLEPinpadComponent::increment_hotp_counter() {
  uint32_t nextVal = this->get_current_hotp_counter() + 1;
  if (this->hotp_counter_.save(&nextVal)) {
    this->hotp_counter_characteristic_->set_value(std::to_string(nextVal));
    return nextVal;
  }
  ESP_LOGW(TAG, "Failed to save hotp counter");
  return nextVal - 1;
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
      if (this->status_indicator_ != nullptr) {
        this->status_indicator_->turn_off();
      }
      break;
    }
    case STATE_PIN_ACCEPTED : {
      if (this->status_indicator_ != nullptr) {
        this->status_indicator_->turn_on();
      }
      if (this->security_mode_ == SECURITY_MODE_HOTP) {
        this->increment_hotp_counter();
      }
      if (now - this->current_state_start_ > VALIDATION_STATE_HOLD_MILLIS) {
        this->set_state_(STATE_IDLE);
      }
      break;
    }
    case STATE_PIN_REJECTED: {
      if (now - this->current_state_start_ > VALIDATION_STATE_HOLD_MILLIS) {
        this->set_state_(STATE_IDLE);
      }
      break;
    }
  }
}

void ESP32BLEPinpadComponent::set_state_(State state) {
  if (state == this->state_) {
    return;
  }

  ESP_LOGV(TAG, "Setting state: %d", state);
  this->state_ = state;
  this->current_state_start_ = millis();

  // Publish to status BLE characteristic.
  switch (state) {
    case STATE_STOPPED:
      this->status_->set_value(std::string("stopped"));
      break;
    case STATE_IDLE:
      this->status_->set_value(std::string("idle"));
      break;
    case STATE_PIN_ACCEPTED:
      this->status_->set_value(std::string("accepted"));
      break;
    case STATE_PIN_REJECTED:
      this->status_->set_value(std::string("rejected"));
      break;
    default:
      assert(false);
  }
  this->status_->notify();

  // Publish to internal triggers.
  this->state_callback_.call();
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

  ESP_LOGD(TAG, "Processing pin message - %s", format_hex_pretty(this->incoming_data_).c_str());
  if (data_len > INPUT_MAX_LEN) {
    ESP_LOGV(TAG, "Too much data came in, or malformed resetting buffer...");
  } else {
    ESP_LOGV(TAG, "Processing pin input!");
    const std::string candidate_pin(this->incoming_data_.begin(), this->incoming_data_.end());
    this->validate_pin_(candidate_pin);
  }
  this->incoming_data_.clear();
}

void ESP32BLEPinpadComponent::validate_pin_(std::string pin) {
  // TODO(mikey): Does ESPHOME support constant time compare?
  std::string expected_pin;

  switch (this->security_mode_) {
    case SECURITY_MODE_HOTP: {
      std::vector<uint8_t> password_vector(this->secret_passcode_.begin(), this->secret_passcode_.end());
      uint32_t expected_otp_intval = otp::hotp_generate(
        password_vector.data(),
        password_vector.size(),
        this->get_current_hotp_counter(),
        6
      );
      expected_pin = std::to_string(expected_otp_intval);
      break;
    }
    case SECURITY_MODE_TOTP: {
      std::vector<uint8_t> password_vector(this->secret_passcode_.begin(), this->secret_passcode_.end());
      uint32_t expected_otp_intval = otp::totp_generate(
        password_vector.data(),
        password_vector.size()
      );
      expected_pin = std::to_string(expected_otp_intval);
      break;
    }
    case SECURITY_MODE_NONE: {
      expected_pin = this->secret_passcode_;
      break;
    }
  }

  ESP_LOGD(TAG, "Expected pin: %s", expected_pin.c_str());

  if (pin == expected_pin) {
    ESP_LOGD(TAG, "Pin accepted");
    this->set_state_(STATE_PIN_ACCEPTED);
  } else {
    ESP_LOGD(TAG, "Pin rejected");
    this->set_state_(STATE_PIN_REJECTED);
  }
}

void ESP32BLEPinpadComponent::on_client_disconnect() { 
  this->incoming_data_.clear();
  this->set_state_(STATE_IDLE);
};

}  // namespace esp32_ble_pinpad
}  // namespace esphome

#endif
