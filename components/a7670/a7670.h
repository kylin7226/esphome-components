
class A7670Component : public Component, public uart::UARTDevice {
 public:
  // 初始化模块配置
  void setup() override;

  // 主循环逻辑，处理串口数据
  void loop() override;

  // 更新网络信息（信号强度、注册状态、运营商信息）
  void update_network_info_();

  // 列出可用运营商
  void list_operators();

  // 选择指定运营商
  void select_operator(const std::string &operator_name);

  // 发送短信到指定号码
  void send_sms(const std::string &number, const std::string &message);

  // 重启模块
  void restart_module_();

  // 发送自定义 AT 指令
  void send_at_command(const std::string &command);

 protected:
  // 信号强度传感器
  sensor::Sensor *signal_strength_sensor_{nullptr};

  // 网络状态传感器
  sensor::Sensor *network_status_sensor_{nullptr};

  // 短信接收传感器
  text_sensor::TextSensor *sms_sensor_{nullptr};

  // 运营商信息传感器
  text_sensor::TextSensor *carrier_sensor_{nullptr}; 

  // 模块电源键 GPIO 引脚
  GPIOPin *pwk_pin_{nullptr};

  // 解析短信内容
  void parse_sms_(const std::string &buffer); 

  // 解析运营商信息
  void parse_carrier_info_(const std::string &buffer); 

  // 发送 AT 指令
  void send_command_(const std::string &command);

  // 解析串口接收到的每一行数据
  void parse_line_(std::string &buffer);
};
