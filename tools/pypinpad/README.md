# pypinpad

A small debug CLI for [esp32-ble-pinpad](https://github.com/mik3y/esp32-ble-pinpad).
It discovers a pinpad device over BLE and sends it a PIN, so you can exercise a
device without building a full client.

## Install

[`uv`](https://docs.astral.sh/uv/) is required.

```
uv sync
```

## Usage

```
uv run pypinpad list   # discover nearby pinpad devices
uv run pypinpad pin    # send a PIN to a device (prompts for the password)
```

Example:

```
$ uv run pypinpad pin
Searching for BLE devices ...
Found device ble-pinpad-example (9A4935AD-7909-4E76-8481-1D42C213B689). Use it? [Y/n]: y
Device security mode: hotp
Please enter the device password: changeme
Current hotp counter: 13
Sending pin "100596" to device ...
```

## Development

Lint/format with [`ruff`](https://docs.astral.sh/ruff/):

```
uv run ruff check
uv run ruff format
```
