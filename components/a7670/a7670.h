#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/gpio.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/button/button.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace a7670 {

class A7670Component : public Component, public uart::UARTDevice {
 public:
  A7670Component(esphome::uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  void setup() override;
  void loop() override;
  void dump_config() override;

  void send_sms(const std::string &number, const std::string &message);
  void receive_sms();
  void query_network_info();
  void query_available_operators();
  void select_operator(const std::string &operator_name);
  void restart_module();
  std::string send_custom_at_command(const std::string &command);

  void set_pwk_pin(GPIOPin *pin) { this->pwk_pin_ = pin; }

  void register_button(button::Button *button) { this->restart_button_ = button; }
  void register_sensor(sensor::Sensor *sensor, const std::string &type);
  void register_text_sensor(text_sensor::TextSensor *text_sensor, const std::string &type);

 protected:
  void send_at_command(const std::string &command);
  std::string read_response();
  std::string gsm_to_utf8(const std::string &gsm);
  std::string parse_sms(const std::string &response);
  std::string parse_registration_status(const std::string &response);
  std::string parse_operator_code(const std::string &response);
  float parse_signal_strength(const std::string &response);

  GPIOPin *pwk_pin_{nullptr};
  button::Button *restart_button_{nullptr};
  sensor::Sensor *registration_status_sensor_{nullptr};
  sensor::Sensor *signal_strength_sensor_{nullptr};
  text_sensor::TextSensor *operator_text_sensor_{nullptr};
  text_sensor::TextSensor *sms_text_sensor_{nullptr};
};

}  // namespace a7670
}  // namespace esphome
