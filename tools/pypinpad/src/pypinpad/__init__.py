import asyncio
import rich_click as click
from bleak import BleakScanner, BleakClient
from bleak.exc import BleakError
import pyotp
import base64

SECURITY_MODE_NONE = "none"
SECURITY_MODE_HOTP = "hotp"
SECURITY_MODE_TOTP = "totp"

PINPAD_SERVICE_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900000"
PINPAD_STATUS_CHR_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900001"
PINPAD_RPC_COMMAND_CHR_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900002"
PINPAD_SECURITY_MODE_CHR_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900003"
PINPAD_HOTP_COUNTER_CHR_UUID = "0003cc02-25ce-4e26-a32f-8c1bfa900004"


def debug(msg):
    click.secho(msg, dim=True)


async def get_security_mode(client):
    rawval = await client.read_gatt_char(PINPAD_SECURITY_MODE_CHR_UUID)
    return rawval.decode()


async def get_hotp_counter(client):
    rawval = await client.read_gatt_char(PINPAD_HOTP_COUNTER_CHR_UUID)
    return int(rawval.decode())


async def connect_device(device, attempts=3, timeout=20.0):
    # Connect using the BLEDevice object from the scan rather than its address
    # string: on macOS an address is an opaque CoreBluetooth UUID, and passing
    # it forces bleak to re-scan to resolve it (which hangs). CoreBluetooth also
    # likes to time out on the first attempt, so retry a few times.
    client = BleakClient(device, timeout=timeout)
    last_error = None
    for attempt in range(1, attempts + 1):
        try:
            await client.connect()
            return client
        except (TimeoutError, BleakError) as err:
            last_error = err
            debug(f"Connection attempt {attempt}/{attempts} failed: {err!r}")
    raise click.ClickException(
        f"Could not connect to {device.name or device.address}: {last_error}. "
        "Make sure it is powered on, in range, and not already connected to "
        "another app or a Home Assistant Bluetooth proxy."
    )


async def perform_pinin(device_address=None, password=None):
    device = await select_device(device_address=device_address)
    client = await connect_device(device)
    try:
        mode = await get_security_mode(client)
        debug(f"Device security mode: {mode}")

        if mode in (SECURITY_MODE_HOTP, SECURITY_MODE_TOTP):
            if not password:
                password = click.prompt("Please enter the device password")

        pin = None
        if mode == SECURITY_MODE_HOTP:
            hotp_counter = await get_hotp_counter(client)
            debug(f"Current hotp counter: {hotp_counter}")
            hotp = pyotp.HOTP(base64.b32encode(password.encode("utf-8")))
            pin = hotp.at(hotp_counter)
        elif mode == SECURITY_MODE_TOTP:
            totp = pyotp.TOTP(base64.b32encode(password.encode("utf-8")))
            pin = totp.now()
        else:
            pin = click.prompt("Please enter the pin")

        if pin is None:
            raise ValueError("BUG")

        def status_change_callback(sender, data):
            status = data.decode()
            click.echo('Result: ' + click.style(status, bold=True))
            asyncio.create_task(client.disconnect())

        await client.start_notify(
            PINPAD_STATUS_CHR_UUID, status_change_callback,
        )

        debug(f'Sending pin "{pin}" to device ...')
        command_bytes = f"{pin}".encode("ascii")
        await client.write_gatt_char(
            PINPAD_RPC_COMMAND_CHR_UUID, command_bytes, response=True
        )

        # Wait for `status_change_callback` to fire and close our client
        # connection.
        while client.is_connected:
            await asyncio.sleep(0.1)
    finally:
        if client.is_connected:
            await client.disconnect()


async def select_device(device_address=None):
    selected_device = None
    async for device in find_devices(device_address=device_address):
        if device_address and device.address == device_address:
            selected_device = device
        elif click.confirm(
            "Found device "
            + click.style(device.name, fg="blue", bold=True)
            + click.style(f" ({device.address})", dim=True)
            + ". Use it?",
            default=True,
        ):
            selected_device = device
            break
    if not selected_device:
        raise click.ClickException("No device found!")
    return selected_device


async def list_devices():
    found = False
    async for device in find_devices():
        found = True
        click.echo(
            click.style(device.name or "(unknown)", fg="blue", bold=True)
            + click.style(f" ({device.address})", dim=True)
        )
    if not found:
        click.echo("No pinpad devices found.")


async def find_devices(device_address=None):
    debug("Searching for BLE devices ...")
    devices = await BleakScanner.discover(service_uuids=[PINPAD_SERVICE_UUID])
    for device in devices:
        address = device.address
        if device_address and address != device_address:
            continue
        try:
            yield device
        except asyncio.TimeoutError:
            pass


@click.group()
def cli():
    pass


@cli.command()
def list():
    """List discoverable pinpad devices."""
    asyncio.run(list_devices())


@cli.command()
@click.option("--device_address", required=False, help="Specific device to use")
@click.option(
    "--password",
    required=False,
    help="For TOTP/HOTP, the password (will prompt if not given)",
)
def pin(device_address=None, password=None):
    asyncio.run(perform_pinin(device_address=device_address, password=password))


if __name__ == "__main__":
    cli()
