import esphome.codegen as cg
from esphome import automation
import esphome.config_validation as cv
from esphome.components import sensor, i2c
from esphome.const import (
    STATE_CLASS_MEASUREMENT,
    UNIT_METER,
    ICON_ARROW_EXPAND_VERTICAL,
    CONF_ADDRESS,
    CONF_MODE,
    CONF_ID,
    CONF_ENABLE_PIN,
)
from esphome import pins

DEPENDENCIES = ["i2c"]

vl53l3cx_ns = cg.esphome_ns.namespace("vl53l3cx")
VL53L3CXSensor = vl53l3cx_ns.class_(
    "VL53L3CXSensor", sensor.Sensor, cg.PollingComponent, i2c.I2CDevice
)

# Modes of operation (i.e which distance figures to report)
ReportMode = vl53l3cx_ns.namespace("ReportMode").enum("ReportMode")
REPORT_MODES = {
    "Furtherest": ReportMode.Furtherest,
    "Closest": ReportMode.Closest,
}
validate_report_mode = cv.enum(REPORT_MODES)

# Actions
VL53L3CXCalibrateAction = vl53l3cx_ns.class_(
    "VL53L3CXCalibrateAction", automation.Action
)

def check_keys(obj):
    if obj[CONF_ADDRESS] != 0x29 and CONF_ENABLE_PIN not in obj:
        msg = "Address other then 0x29 requires enable_pin definition to allow sensor\r"
        msg += "re-addressing. Also if you have more then one VL53 device on the same\r"
        msg += "i2c bus, then all VL53 devices must have enable_pin defined."
        raise cv.Invalid(msg)
    return obj

CONFIG_SCHEMA = cv.All(
    sensor.sensor_schema(
        VL53L3CXSensor,
        unit_of_measurement=UNIT_METER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=3,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend({
        cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_MODE): validate_report_mode,
    })
    .extend(cv.polling_component_schema("0.5s"))
    .extend(i2c.i2c_device_schema(0x29)),
    check_keys,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    if CONF_ENABLE_PIN in config:
        enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(enable))
    if (mode := config.get(CONF_MODE)) is not None:
        cg.add(var.set_report_mode(mode))

    await i2c.register_i2c_device(var, config)



VL53L3CX_CALIBRATE_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(VL53L3CXSensor),
    }
)

@automation.register_action("vl53l3cx.calibrate", VL53L3CXCalibrateAction, VL53L3CX_CALIBRATE_ACTION_SCHEMA)
async def calibrate_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
