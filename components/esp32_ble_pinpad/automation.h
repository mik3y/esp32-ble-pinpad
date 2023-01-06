#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esp32_ble_pinpad_component.h"

namespace esphome {
namespace esp32_ble_pinpad {

class PinpadAcceptedTrigger : public Trigger<> {
 public:
  PinpadAcceptedTrigger(ESP32BLEPinpadComponent *pinpad) {
    pinpad->add_on_state_callback([this, pinpad]() {
      if (pinpad->is_accepted()) {
        this->trigger();
      }
    });
  }
};

class PinpadRejectedTrigger : public Trigger<> {
 public:
  PinpadRejectedTrigger(ESP32BLEPinpadComponent *pinpad) {
    pinpad->add_on_state_callback([this, pinpad]() {
      if (pinpad->is_rejected()) {
        this->trigger();
      }
    });
  }
};

}  // namespace esp32_ble_pinpad
}  // namespace esphome
