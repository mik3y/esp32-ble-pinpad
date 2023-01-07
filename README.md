# ESP32 Bluetooth LE (BLE) Pinpad

This an ESPHome custom component that exposes a "pin pad" BluetoothLE server.

It is a component which can trigger other things (such as an `output` device) when the correct value is received over BLE.

> **Status**: 🚨 Experimental! Use at your own risk.

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [Why?](#why)
- [Usage](#usage)
- [Configuration options](#configuration-options)
- [Protocol (how it works)](#protocol-how-it-works)
- [Testing](#testing)
- [Security](#security)
- [Developer Instructions](#developer-instructions)
  - [Using local sources in esphome](#using-local-sources-in-esphome)
  - [Updating `README.md`](#updating-readmemd)
  - [References and Reading](#references-and-reading)
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
      url: https://github.com/mik3y/esp32-ble-pinpad
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
  secret_passcode: changeme
  security_mode: hotp
  status_indicator: onboard_status_led
  on_pinpad_accepted:
    - logger.log: "Rad! Correct pin was given!"
  on_pinpad_rejected:
    - logger.log: "Bummer! Incorrect pin was given."
```

Of course, you'll probably want to do something other than log when the pin comes in. You can use [any availabe ESPHome trigger](https://esphome.io/guides/automations.html#all-triggers) here. For example, if you have a garage door set up as a cover, you could do:

```yml
  on_pinpad_accepted:
    - logger.log: "Got pin over BLE, opening door."
    - cover.toggle: garage_door
```

## Configuration options

You can provide these options in the `esp32_ble_pinpad` yaml block:

* **`secret_passcode`** (required): The secret passcode. Follow strong password rules for best security (i.e. longer values are better).
* **`security_mode`** (required): One of `none`, `hotp`, or `totp`. If in doubt, `hotp` is recommended.
    * **Warning:** Using `none` is highly insecure, as it means the passcode will be exposed over-the-wire. See _Security_ for further information.
* **`status_indicator`** (optional): An `output` to use for blinking status.
* **`on_pinpad_accepted`** (optional): Trigger(s) to fire when a PIN is accepted.
* **`on_pinpad_rejected`** (optional): Trigger(s) to fire when a PIN is rejected.


## Protocol (how it works)

Two configuration factors are set by you and flashed into the device:

* `secret_passcode`: The value you will use and give to anyone wishing to "pin in".
* `security_mode`: One of `none`, `hotp`, or `totp`, this determines how the passcode is used.

To authenticate ("pin in"), a client performs the following steps:

1. Discover the device and create a BLE connection to the pinpad service.
2. Read the `security_mode` setting of the device. This is available at the `PINPAD_SECURITY_MODE_CHR` BLE GATT characteristic.
3. Compute the secret to send:
    1. If `security_mode = none`, the value of `secret_passcode` is sent in full and in the clear. _This is the least secure option and is not recommended._
    2. If `security_mode = hotp`, the client must read the current HOTP counter from `PINPAD_HOTP_COUNTER_CHR`, then generate and send a 6-digit HOTP code based on that value and using `secret_passcode` as the key.
    3. If `security_mode = totp`, the client must generate and send a 6-digit TOTP code based on the current time and using `secret_passcode` as the key.
4. Optionally, read `PINPAD_STATUS_CHR` to determine whether the operation succeeded.

## Testing

A debug program is included at `tools/pypinpad/pypinpad.py` which can be used to send a pin to a locally-discoverable BLE device.

To install the tool's dependencies, `poetry` is required.

```
cd tools/pypinpad
poetry install
```

Example usage:

```
$ poetry shell
(pypinpad-py3.10) $ python pypinpad.py pin
Searching for BLE devices ...
Found device ble-pinpad-example (9A4935AD-7909-4E76-8481-1D42C213B689). Use it? [Y/n]: y
Device security mode: hotp
Please enter the device password: changeme
Current hotp counter: 13
Sending pin "100596" to device ...
```


## Security

**Warning:** You should consider this component **totally insecure**. No warranty. Please review `LICENSE.txt`.

When `security_mode` is `hotp` or `totp`, the secret passcode is never revealed on the wire. Captured payloads should not be replayable: In the case of `hotp`, the counter is incremented after every successful authentication and persisted to flash. `totp` follows a standard 30-second window.


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


## Help, Feedback, and Ideas

Please open an issue on GitHub!

Here are some ideas I haven't gotten around to yet.

- BLE interface for adding/revoking pins (using some sort of admin pin).
- BLE interface for reading attempt history.

## License & Credit

Written by **@mik3y**. Licensed under GPLv3 (same as esphome).

Greatly inspired by & indebted to the built-in [esp32_improv component](https://github.com/esphome/esphome/blob/dev/esphome/components/esp32_improv).
