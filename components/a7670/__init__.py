import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import uart, button, sensor, text_sensor
from esphome import pins
from esphome.const import CONF_ID, CONF_PIN, CONF_COMMAND

DEPENDENCIES = ['uart']

a7670_ns = cg.esphome_ns.namespace('a7670')
A7670Component = a7670_ns.class_('A7670Component', cg.Component, uart.UARTDevice)
A7670RestartButton = a7670_ns.class_('A7670RestartButton', button.Button, cg.Component)

CONF_BUTTON = "button"
CONF_SENSOR = "sensor"
CONF_TEXT_SENSOR = "text_sensor"
CONF_TYPE = "type"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(A7670Component),
    cv.Required(CONF_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_BUTTON): cv.Schema({
        cv.GenerateID(): cv.declare_id(button.Button),
    }),
    cv.Optional(CONF_SENSOR): cv.ensure_list(cv.Schema({
        cv.GenerateID(): cv.declare_id(sensor.Sensor),
        cv.Required(CONF_TYPE): cv.string,
    })),
    cv.Optional(CONF_TEXT_SENSOR): cv.ensure_list(cv.Schema({
        cv.GenerateID(): cv.declare_id(text_sensor.TextSensor),
        cv.Required(CONF_TYPE): cv.string,
    })),
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    # 初始化 UARTComponent
    uart_component = cg.new_Pvariable(config[CONF_ID], config)

    var = cg.new_Pvariable(config[CONF_ID], uart_component)
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    pin = yield cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pwk_pin(pin))

    if CONF_BUTTON in config:
        button_conf = config[CONF_BUTTON]
        button_var = cg.new_Pvariable(button_conf[CONF_ID])
        cg.add(var.register_button(button_var))
        yield cg.register_component(button_var, button_conf)

    if CONF_SENSOR in config:
        for sensor_conf in config[CONF_SENSOR]:
            sensor_var = cg.new_Pvariable(sensor_conf[CONF_ID])
            cg.add(var.register_sensor(sensor_var, sensor_conf[CONF_TYPE]))
            yield cg.register_component(sensor_var, sensor_conf)
            yield sensor.register_sensor(sensor_var, sensor_conf)

    if CONF_TEXT_SENSOR in config:
        for text_sensor_conf in config[CONF_TEXT_SENSOR]:
            text_sensor_var = cg.new_Pvariable(text_sensor_conf[CONF_ID])
            cg.add(var.register_text_sensor(text_sensor_var, text_sensor_conf[CONF_TYPE]))
            yield cg.register_component(text_sensor_var, text_sensor_conf)
            yield text_sensor.register_text_sensor(text_sensor_var, text_sensor_conf)
