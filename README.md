# ESP32 Bluetooth LE (BLE) Pinpad

This an ESPHome custom component that exposes a "pin pad" BluetoothLE server.

It is a component which can trigger other things (such as an `output` device) when the correct value is received over BLE.

> **Status**: 🚨 Experimental! Use at your own risk.

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [Why?](#why)
- [Usage](#usage)
- [Configuration options](#configuration-options)
- [Developer Instructions](#developer-instructions)
  - [Using local sources in esphome](#using-local-sources-in-esphome)
  - [Updating `README.md`](#updating-readmemd)
  - [References and Reading](#references-and-reading)
- [Security Considerations](#security-considerations)
- [Help, Feedback, and Ideas](#help-feedback-and-ideas)
- [License & Credit](#license--credit)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Why?

A "pin pad" is a simple input device which receives a string over BLE, checks it against the valid PIN(s), and triggers an "accepted" or "rejected" callback within ESPHome.

You could use it to make a switch, lock, or other device which is unlocked via BLE.

I wrote it because I had exactly that need, and specifically wanted something that didn't require wifi in order to work. Oh, and of course, I was bored/curious.


## Usage

In your esphome YML, first import this component:

```yml
external_components:
  - source:
      type: git
      url: https://github.com/mik3y/esp32_ble_pinpad
      ref: main
    components: [ esp32_ble_pinpad ]
```

Next, and optionally, set up a status LED.

```yml
output:
  - platform: ledc
    pin: GPIO2
    id: onboard_status_led
```

Finally, add the component itself:

```yml
esp32_ble_pinpad:
  static_secret_pin: "1234"
  status_indicator: onboard_status_led
  on_pinpad_accepted:
    - logger.log: "Rad! Correct pin was given!"
  on_pinpad_rejected:
    - logger.log: "Bummer! Incorrect pin was given."
```

Note that `static_secret_pin` must be a string. You can make it all numbers, but you'll need to quote it if so (yaml is fun).

Of course, you'll probably want to do something other than log when the pin comes in. You can use [any availabe ESPHome trigger](https://esphome.io/guides/automations.html#all-triggers) here. For example, if you have a garage door set up as a cover, you could do:

```yml
  on_pinpad_accepted:
    - logger.log: "Got pin over BLE, opening door."
    - cover.toggle: garage_door
```

## Configuration options

You can provide these options in the `esp32_ble_pinpad` yaml block:

* **`static_secret_pin`** (required): The pin value which will be accepted.
* **`status_indicator`** (optional): An `output` to use for blinking status.
* **`on_pinpad_accepted`** (optional): Trigger(s) to fire when a PIN is accepted.
* **`on_pinpad_rejected`** (optional): Trigger(s) to fire when a PIN is rejected.

## Developer Instructions

Notes for folks working on the component itself.

### Using local sources in esphome

See `example.yml` for how to pull the component in.

### Updating `README.md`

Use `doctoc` to update the table of contents:

```
npm install -g doctoc
doctoc --notitle --github README.md
```

### References and Reading

Here are some things I found helpful:

* An overview of BLE (protocol & stack): https://novelbits.io/bluetooth-low-energy-ble-complete-guide
* An unrelated ble stack bugreport, but with great discussion: https://github.com/espressif/arduino-esp32/issues/1038
* BLE officially assigned numbers (PDF): https://btprodspecificationrefs.blob.core.windows.net/assigned-numbers/Assigned%20Number%20Types/Assigned%20Numbers.pdf

## Security Considerations

**Warning:** You should consider this component **totally insecure**.

It is still just a hack, and I have made no attempt to validate its security properties. For example, someone with the means to sniff nearby BLE traffic could likely capture and replay your PIN.

No warranty. Please review `LICENSE.txt`.

## Help, Feedback, and Ideas

Please open an issue on GitHub!

Here are some ideas I haven't gotten around to yet.

- Implement TOTP and accept one-time pins.
- BLE interface for adding/revoking pins (using some sort of admin pin).
- BLE interface for reading attempt history.

## License & Credit

Written by **@mik3y**. Licensed under GPLv3 (same as esphome).

Greatly inspired by & indebted to the built-in [esp32_improv component](https://github.com/esphome/esphome/blob/dev/esphome/components/esp32_improv).
