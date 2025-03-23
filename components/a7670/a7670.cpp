#include "a7670.h"
#include "esphome/core/log.h"
#include <iomanip>
#include <sstream>
#include <ctime>

esphome::uart::UARTComponent *uart_component;
esphome::a7670::A7670Component *a7670_component;

namespace esphome {
namespace a7670 {

static const char *TAG = "a7670";

void A7670Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up A7670...");
  this->send_at_command("ATE0");
  this->send_at_command("AT+CMGF=1");  // 设置短信为文本模式
  this->send_at_command("AT+CSCS=\"GSM\"");  // 设置字符集为 GSM
  this->send_at_command("AT+CNMI=2,1,0,0,0");
  this->send_at_command("AT+CFUN=1,1");
}

void A7670Component::loop() {
  this->receive_sms();
}

void A7670Component::dump_config() {
  ESP_LOGCONFIG(TAG, "A7670:");
  LOG_PIN("  PWK Pin: ", this->pwk_pin_);
  if (this->restart_button_ != nullptr) {
    LOG_BUTTON("  Restart Button: ", "button", this->restart_button_);
  }
  if (this->registration_status_sensor_ != nullptr) {
    LOG_SENSOR("  Registration Status Sensor: ", "sensor", this->registration_status_sensor_);
  }
  if (this->signal_strength_sensor_ != nullptr) {
    LOG_SENSOR("  Signal Strength Sensor: ", "sensor", this->signal_strength_sensor_);
  }
  if (this->operator_text_sensor_ != nullptr) {
    LOG_TEXT_SENSOR("  Operator Text Sensor: ", "text_sensor", this->operator_text_sensor_);
  }
  if (this->sms_text_sensor_ != nullptr) {
    LOG_TEXT_SENSOR("  SMS Text Sensor: ", "text_sensor", this->sms_text_sensor_);
  }
}

void A7670Component::send_sms(const std::string &number, const std::string &message) {
  this->send_at_command("AT+CMGS=\"" + number + "\"");
  this->send_at_command(message + "\x1A");
  delay(5000);  // 等待短信发送完成
  this->send_at_command("AT+CMGD=1,4");  // 删除所有短信
}

void A7670Component::receive_sms() {
  this->send_at_command("AT+CMGL=\"ALL\"");
  std::string response = this->read_response();
  if (this->sms_text_sensor_ != nullptr) {
    std::string parsed_sms = parse_sms(response);
    this->sms_text_sensor_->publish_state(parsed_sms);
  }
}

void A7670Component::query_network_info() {
  this->send_at_command("AT+CREG?");
  std::string reg_response = this->read_response();
  if (this->registration_status_sensor_ != nullptr) {
    std::string reg_status = parse_registration_status(reg_response);
    this->registration_status_sensor_->publish_state(std::stof(reg_status));
  }

  this->send_at_command("AT+COPS?");
  std::string ops_response = this->read_response();
  if (this->operator_text_sensor_ != nullptr) {
    std::string operator_name = parse_operator_code(ops_response);
    this->operator_text_sensor_->publish_state(operator_name);
  }

  this->send_at_command("AT+CSQ");
  std::string csq_response = this->read_response();
  if (this->signal_strength_sensor_ != nullptr) {
    float signal_strength = parse_signal_strength(csq_response);
    this->signal_strength_sensor_->publish_state(signal_strength);
  }
}

void A7670Component::query_available_operators() {
  this->send_at_command("AT+COPS=?");
  std::string response = this->read_response();
  // 解析并处理可用运营商
}

void A7670Component::select_operator(const std::string &operator_name) {
  this->send_at_command("AT+COPS=1,0,\"" + operator_name + "\"");
}

void A7670Component::restart_module() {
  if (this->pwk_pin_ != nullptr) {
    this->pwk_pin_->digital_write(false);
    delay(1000);
    this->pwk_pin_->digital_write(true);
    ESP_LOGD(TAG, "Restarting A7670 module");
  }
}

void A7670Component::send_at_command(const std::string &command) {
  ESP_LOGD(TAG, "Sending AT command: %s", command.c_str());
  this->write_str((command + "\r\n").c_str());
}

std::string A7670Component::read_response() {
  std::string response;
  while (this->available()) {
    response += (char) this->read();
  }
  ESP_LOGD(TAG, "Received response: %s", response.c_str());
  return response;
}

std::string A7670Component::gsm_to_utf8(const std::string &gsm) {
  std::string utf8;
  for (char c : gsm) {
    if (c >= 0x20 && c <= 0x7F) {
      utf8 += c;
    } else if (c == 0x0A) {
      utf8 += '\n';
    } else if (c == 0x0D) {
      utf8 += '\r';
    } else {
      utf8 += '?';  // 未知字符替换为 '?'
    }
  }
  return utf8;
}

std::string A7670Component::parse_sms(const std::string &response) {
  std::string result;
  size_t pos = response.find("\r\n");
  if (pos != std::string::npos) {
    std::string header = response.substr(0, pos);
    std::string body = response.substr(pos + 2);
    size_t start = header.find("+CMGL: ");
    if (start != std::string::npos) {
      size_t end = header.find(",", start);
      std::string status = header.substr(start + 7, end - start - 7);
      start = header.find("\"", end + 1);
      end = header.find("\"", start + 1);
      std::string phone_number = header.substr(start + 1, end - start - 1);
      start = header.find("\"", end + 1);
      end = header.find("\"", start + 1);
      std::string time = header.substr(start + 1, end - start - 1);
      std::string message = gsm_to_utf8(body);

      result = "时间: " + time + ", 来自: " + phone_number + ", 内容: " + message;
    }
  }
  return result;
}

std::string A7670Component::parse_registration_status(const std::string &response) {
  // 解析注册状态
  // 示例返回值："+CREG: 0,1"
  int status_code = 1; // 解析逻辑
  switch (status_code) {
    case 0: return "0";
    case 1: return "1";
    case 2: return "2";
    case 3: return "3";
    case 5: return "5";
    default: return "0";
  }
}

std::string A7670Component::parse_operator_code(const std::string &response) {
  // 解析运营商代码
  // 示例返回值："+COPS: 0,0,"46000""
  std::string operator_code = "46000"; // 解析逻辑
  if (operator_code == "46000") return "中国移动";
  if (operator_code == "46001") return "中国联通";
  if (operator_code == "46003") return "中国电信";
  if (operator_code == "46006") return "中国联通";
  if (operator_code == "46011") return "中国电信";
  if (operator_code == "46022") return "中国广电";
  if (operator_code == "23410") return "giffgaff";
  return "未知运营商";
}

float A7670Component::parse_signal_strength(const std::string &response) {
  // 解析信号强度
  // 示例返回值："+CSQ: 15,99"
  int rssi = 15; // 解析逻辑
  return 2 * rssi - 113; // 将RSSI转换为dBm
}

std::string A7670Component::send_custom_at_command(const std::string &command) {
  this->send_at_command(command);
  std::string response = this->read_response();
  return response;
}

void A7670Component::register_sensor(sensor::Sensor *sensor, const std::string &type) {
  if (type == "registration_status") {
    this->registration_status_sensor_ = sensor;
  } else if (type == "signal_strength") {
    this->signal_strength_sensor_ = sensor;
  }
}

void A7670Component::register_text_sensor(text_sensor::TextSensor *text_sensor, const std::string &type) {
  if (type == "operator") {
    this->operator_text_sensor_ = text_sensor;
  } else if (type == "sms") {
    this->sms_text_sensor_ = text_sensor;
  }
}

}  // namespace a7670
}  // namespace esphome
