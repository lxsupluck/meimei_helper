#pragma once

#include <cstdint>
#include "Types.h"

namespace meimei {
namespace config {

// ============================================================
// 日志
// ============================================================
constexpr const char* LOG_FILE_PATH = "./meimei.log";
constexpr LogLevel    LOG_LEVEL     = LogLevel::DEBUG;

// ============================================================
// RS485 / Modbus 串口
// ============================================================
constexpr const char* MODBUS_DEVICE    = "/dev/ttyUSB0";
constexpr int         MODBUS_BAUD      = 9600;
constexpr char        MODBUS_PARITY    = 'N';
constexpr int         MODBUS_DATA_BITS = 8;
constexpr int         MODBUS_STOP_BITS = 1;

// ============================================================
// 温湿度传感器
// ============================================================
constexpr int      SENSOR_SLAVE_ID   = 1;
constexpr int      SENSOR_TEMP_REG   = 0;
constexpr int      SENSOR_HUMI_REG   = 1;
constexpr float    SENSOR_TEMP_SCALE = 0.1f;
constexpr float    SENSOR_HUMI_SCALE = 0.1f;
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;

// ============================================================
// 告警阈值
// ============================================================
constexpr float ALARM_TEMP_HIGH = 29.0f;
constexpr float ALARM_TEMP_LOW  = 5.0f;

// ============================================================
// CSV 数据记录
// ============================================================
constexpr const char* CSV_DIR          = "./log";
constexpr uint32_t    CSV_INTERVAL_S   = 60;
constexpr int         CSV_MAX_FILES    = 30;

// ============================================================
// 看门狗（预留）
// ============================================================
constexpr uint32_t WDT_INTERVAL_MS = 1000;
constexpr int      WDT_GPIO_PIN    = 17;

// ============================================================
// MQTT（预留）
// ============================================================
constexpr const char* MQTT_HOST      = "mqtt.example.com";
constexpr int         MQTT_PORT      = 1883;
constexpr const char* MQTT_CLIENT_ID = "meimei_helper_001";
constexpr const char* MQTT_TOPIC     = "factory/line1/device001";

// ============================================================
// 语音（预留）
// ============================================================
constexpr const char* STT_API_URL = "https://api.example.com/stt";
constexpr const char* LLM_API_URL = "https://api.example.com/llm";
constexpr const char* TTS_API_URL = "https://api.example.com/tts";

}  // namespace config
}  // namespace meimei
