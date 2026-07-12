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
constexpr int      THSENS_SLAVE_ID   = 1;
constexpr int      SENSOR_TEMP_REG   = 0;
constexpr int      SENSOR_HUMI_REG   = 1;
constexpr float    SENSOR_TEMP_SCALE = 0.1f;
constexpr float    SENSOR_HUMI_SCALE = 0.1f;
constexpr float    SENSOR_TEMP_OFFSET = 0.0f;
constexpr float    SENSOR_HUMI_OFFSET = 0.0f;
constexpr const char* TEMP_UNIT = "℃";
constexpr const char* HUMI_UNIT = "%RH";

// ============================================================
// 数据采集
// ============================================================

constexpr uint32_t SAMPLE_INTERVAL_MS = 100;        //快采样
constexpr uint32_t OUTPUT_INTERVAL_MS = 5000;       //慢采样
constexpr uint32_t TIME_TOLERANCE_MS   = 90;   // 时间容忍
constexpr uint32_t SAMPLE_TOLERANCE_PERCENT = 90;   // 采样容忍
constexpr uint32_t FIREST_WINDOW_BOUNDARY_MS = 1000;       // 首个输出窗口样本数


// ============================================================
// 温湿度度告警阈值
// ============================================================
constexpr float ALARM_TEMP_HIGH = 29.0f;
constexpr float ALARM_TEMP_LOW  = 5.0f;

constexpr float ALARM_HUMI_HIGH = 65.0f;
constexpr float ALARM_HUMI_LOW  = 45.0f;

// ============================================================
// CSV 数据记录
// ============================================================
constexpr const char* CSV_DIR          = "./log";
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
