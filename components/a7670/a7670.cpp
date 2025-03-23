// 初始化模块配置
void A7670Component::setup() {
  if (pwk_pin_) {
    pwk_pin_->setup();
    pwk_pin_->digital_write(true);
  }

  this->send_command_("ATE0"); // 关闭回显
  this->send_command_("AT+CMGF=1"); // 设置短信为文本模式
  this->send_command_("AT+CSCS=\"GSM\""); // 设置字符集为 GSM
  this->send_command_("AT+CNMI=2,1,0,0,0"); // 设置短信接收通知
  this->send_command_("AT+CFUN=1,1"); // 启用完整功能

  this->set_interval(60000, [this]() { this->update_network_info_(); });
}

// 更新网络信息（信号强度、注册状态、运营商信息）
void A7670Component::update_network_info_() {
    this->send_command_("AT+CSQ");  // 查询信号强度
    this->send_command_("AT+CREG?");  // 查询网络注册状态
    this->send_command_("AT+COPS?");  // 查询运营商信息
}

// 解析串口接收到的每一行数据
void A7670Component::parse_line_(std::string &buffer) {
    if (buffer.find("+CSQ:") != std::string::npos) {
        // 解析信号强度
        size_t pos = buffer.find('+CSQ:');
        int rssi = std::stoi(buffer.substr(pos + 5, 2));
        if (signal_strength_sensor_) {
            signal_strength_sensor_->publish_state((rssi * 2) - 113);  // 转换为 dBm
        }
    } else if (buffer.find("+CREG:") != std::string::npos) {
        // 解析网络注册状态
        size_t pos = buffer.find('+CREG:');
        int status = std::stoi(buffer.substr(pos + 7, 1));

        // 网络状态代码与说明的映射关系
        static const std::map<int, std::string> network_status_map = {
            {0, "未注册"}, {1, "已注册"}, {2, "搜索中"},
            {3, "拒绝"}, {5, "漫游"}
        };

        std::string status_str = network_status_map.count(status) ? network_status_map.at(status) : "未知";
        if (network_status_sensor_) {
            network_status_sensor_->publish_state(status_str);
        }
    } else if (buffer.find("+CMT:") != std::string::npos) {
        this->parse_sms_(buffer);
    } else if (buffer.find("+COPS:") != std::string::npos) {
        this->parse_carrier_info_(buffer);
    }
}

// 解析短信内容并发布到传感器
void A7670Component::parse_sms_(const std::string &buffer) {
    size_t num_start = buffer.find('"') + 1;
    size_t num_end = buffer.find('"', num_start);
    size_t time_start = buffer.find(',', num_end) + 2;
    size_t time_end = buffer.find('\n', time_start);

    std::string number = buffer.substr(num_start, num_end - num_start);
    std::string time = buffer.substr(time_start, time_end - time_start);
    std::string content = buffer.substr(time_end + 1);

    // 将 GSM 编码的短信内容转换为 UTF-8 编码
    std::string utf8_content = gsm_to_utf8(content);

    if (sms_sensor_) {
        sms_sensor_->publish_state("手机号：" + number + "\n时间：" + time + "\n内容：" + utf8_content);
    }
}

// 发送短信到指定号码
void A7670Component::send_sms(const std::string &number, const std::string &message) {
    std::string cmd = "AT+CMGS=\"" + number + "\"";
    this->send_command_(cmd);
    delay(500);
    this->write_str((message + "\x1A").c_str());
    delay(2000); // 等待短信发送完成

    // 清理短信存储
    this->send_command_("AT+CMGD=1,4");
}

// 新增：将 GSM 编码字符串转换为 UTF-8 编码
std::string A7670Component::gsm_to_utf8(const std::string &gsm_str) {
    static const std::unordered_map<char, std::string> gsm_to_utf8_map = {
        {'@', "@"}, {'£', "£"}, {'$', "$"}, {'¥', "¥"}, {'è', "è"},
        {'é', "é"}, {'ù', "ù"}, {'ì', "ì"}, {'ò', "ò"}, {'Ç', "Ç"},
        {'\n', "\n"}, {'Ø', "Ø"}, {'ø', "ø"}, {'Å', "Å"}, {'å', "å"}
        // 根据需要扩展映射表
    };

    std::string utf8_result;
    for (char c : gsm_str) {
        if (gsm_to_utf8_map.count(c)) {
            utf8_result += gsm_to_utf8_map.at(c);
        } else {
            utf8_result += c; // 未匹配的字符直接保留
        }
    }
    return utf8_result;
}

// 解析运营商信息并发布到传感器
void A7670Component::parse_carrier_info_(const std::string &buffer) {
    size_t start = buffer.find('"') + 1;
    size_t end = buffer.find('"', start);
    std::string carrier_code = buffer.substr(start, end - start);

    // 添加运营商代码与名称的映射关系
    static const std::map<std::string, std::string> carrier_map = {
        {"46000", "中国移动"}, {"46001", "中国联通"}, {"46003", "中国电信"},
        {"46006", "中国联通"}, {"46011", "中国电信"}, {"46022", "中国广电"},
        {"23410", "giffgaff"}
    };

    std::string carrier_name = carrier_map.count(carrier_code) ? carrier_map.at(carrier_code) : carrier_code;

    if (carrier_sensor_) {
        carrier_sensor_->publish_state(carrier_name);
    }
}

// 列出可用运营商
void A7670Component::list_operators() {
  this->send_command_("AT+COPS=?");
}

// 选择指定运营商
void A7670Component::select_operator(const std::string &operator_name) {
  std::string cmd = "AT+COPS=1,0,\"" + operator_name + "\"";
  this->send_command_(cmd);
}

// 重启模块
void A7670Component::restart_module_() {
  if (pwk_pin_) {
    pwk_pin_->digital_write(false);
    delay(1000);
    pwk_pin_->digital_write(true);
    delay(5000); // 等待模块重启
  }
}