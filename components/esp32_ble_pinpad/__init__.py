from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output, esp32_ble_server
from esphome.const import CONF_ID, CONF_TRIGGER_ID


AUTO_LOAD = ["binary_sensor", "output", "esp32_ble_server"]
CODEOWNERS = ["@mik3y"]
CONFLICTS_WITH = ["esp32_ble_tracker", "esp32_ble_beacon"]
DEPENDENCIES = ["wifi", "esp32"]

CONF_BLE_SERVER_ID = "ble_server_id"
CONF_STATUS_INDICATOR = "status_indicator"
CONF_ON_PINPAD_ACCEPTED = "on_pinpad_accepted"
CONF_ON_PINPAD_REJECTED = "on_pinpad_rejected"

esp32_ble_pinpad_ns = cg.esphome_ns.namespace("esp32_ble_pinpad")
ESP32BLEPinpadComponent = esp32_ble_pinpad_ns.class_(
    "ESP32BLEPinpadComponent", cg.Component, esp32_ble_server.BLEServiceComponent
)

# Triggers
PinpadAcceptedTrigger = esp32_ble_pinpad_ns.class_("PinpadAcceptedTrigger", automation.Trigger.template())
PinpadRejectedTrigger = esp32_ble_pinpad_ns.class_("PinpadRejectedTrigger", automation.Trigger.template())

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ESP32BLEPinpadComponent),
        cv.GenerateID(CONF_BLE_SERVER_ID): cv.use_id(esp32_ble_server.BLEServer),
        cv.Optional(CONF_ON_PINPAD_ACCEPTED): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PinpadAcceptedTrigger),
            }
        ),
        cv.Optional(CONF_ON_PINPAD_REJECTED): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PinpadAcceptedTrigger),
            }
        ),
        cv.Optional(CONF_STATUS_INDICATOR): cv.use_id(output.BinaryOutput),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    ble_server = await cg.get_variable(config[CONF_BLE_SERVER_ID])
    cg.add(ble_server.register_service_component(var))

    # cg.add_define("USE_BLE_PINPAD")

    for conf in config.get(CONF_ON_PINPAD_ACCEPTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_PINPAD_REJECTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    if CONF_STATUS_INDICATOR in config:
        status_indicator = await cg.get_variable(config[CONF_STATUS_INDICATOR])
        cg.add(var.set_status_indicator(status_indicator))
