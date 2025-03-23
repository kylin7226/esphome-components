# 定义组件名称
idf_component_register(
    SRCS "a7670.cpp"
    INCLUDE_DIRS "."
    REQUIRES esphome uart sensor text_sensor
)