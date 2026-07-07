#pragma once  //相当于 #ifndef THPE_H #define TYPES_H,适用范围相对较小

#include <string>
#include <cstdint> //确保int等位数一样，如8位int int8_t
#include <vector>



/*
     霉霉的日志，状态，界面信息收录到霉霉的状态空间，避免与其他库冲突

*/
namespace meimei{

    // =================================
    //  系统日志级别
    // =================================

    //enum用来定义枚举。
    //class使变量被访问使必须加上其作用域 LogLevel::DEBUG
    //同时避免强制类型转换，LogLevel变量只能被LogLevel赋值，避免出现以外的bug。
    enum class LogLevel : uint8_t
    {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3,
        FATAL = 4,
    };


    // =======================================
    //  系统运行状态
    // =======================================
    enum class SystemState : uint8_t
    {
        INIT = 0,
        RUNNING = 1,
        FAULT = 2,
        UPGRADE = 3,
        SHUTDOWN = 4,
    };


    //==============================================================================
    //  告警严重等级
    //=================================================================

    enum class AlarmSeverity : uint8_t
    {   
        INFO = 0,
        WARNING = 1,
        CRITICAL = 2,
    };


    //=====================================
    // 传感器配置
    //====================================
    struct SensorConfig 
    {
        uint32_t    id;
        std::string name;
        uint8_t     slave_id;
        uint16_t    register_addr;      //寄存器起始地址
        uint16_t    register_count;     //寄存器数量
        float       scale_factor;       //缩放系数
        float       offset;             //偏移量
        std::string unit;               //单位
        uint32_t    interval_ms;        //采集间隔
        float       alarm_high;         //高报阈值
        float       alarm_low;          //低报阈值
        uint32_t    filter_window;      //滤波窗口大小
    };

    //=========================================
    //  传感器采集的数据
    //=========================================
    struct SensorSample
    {
        uint32_t    sensor_id;
        float       value;          
        uint64_t    timestamp_ms;   //采集时间戳
        uint8_t     quality;        //数据质量 百分制
    };

    struct AlarmInfo
    {
        uint32_t        sensor_id;
        AlarmSeverity   severity;
        float           current_value;
        float           threshold;
        std::string     message;
        uint64_t        timestamp_ms;
        bool            acknowledged;   //是否已经确认
    };

    //=============================
    //  处理后数据
    //=============================
    struct DataPoint
    {
        uint32_t                sensor_id;
        float                   raw_value;
        float                   filtered_value;
        uint64_t                timestamp_ms;
        uint8_t                 quality;
        std::vector<AlarmInfo> alarms;
    };


    //========================================
    //  HMI (Human-Machine Interface) 人机交互
    //========================================

    // enum class HmiEventType : uint8_t
    // {
    //     DISPLAY_TEXT = 0;
    //     CLEAR_SCREEN = 1;
    //     ALERT = 2;
    //     SET_TITLE =3;
    //     STATUS_LINE = 4;
    // };

    struct SendText
    {
        std::string text;
    };

    struct UserInput
    {
        std::string text;
    };
    

    //===============================
    //  云端指令
    //===============================
    
    struct CloudCommand
    {
        std::string type; //"reboot" /"upgrade" /"set_param" /...
        std::string payload;    //指令参数(JSON)

    };

    //=====================================================
    //  Modbus配置
    //=====================================================
    struct ModbusConfig
    {   
        std::string device;             //串口设备
        int         baud_rate;          //波特率
        char        parity;             // 'N'/'E'/'O'
        int         data_bits;   
        int         stop_bits;
        uint32_t    response_timeout_ms; //相应超时

    };
    
    //=========================================
    //MQTT配置
    //=========================================
    struct MqttConfig
    {
        std::string host;
        int         port;
        std::string client_id;
        std::string username;
        std::string password;
        std::string topic_prefix;
        int         qos;
        uint32_t    keepalive_s;
        uint32_t    reconnect_interval_ms;
    };

    //=====================================
    // 语音配置
    //=====================================
    struct VoiceConfig
    {
        std::string stt_api_url;    //语音转文字api
        std::string llm_api_url;    //大模型api
        std::string tts_api_url;    //文字转语音api
        std::string api_key;        //api密钥
        std::string audio_device;   //录音设备，
        uint32_t    silence_timeout_ms;  //静音超时(stop)
        uint32_t    max_record_ms;  //最大录音时间

    };


    //=======================================
    //系统总配置
    //=======================================
    struct SystemConfig
    {
        ModbusConfig                modbus;
        MqttConfig                  mqtt;
        VoiceConfig                 voice;
        std::vector<SensorConfig>   sensors;
        std::string                 log_file;
        LogLevel                   log_level;
        uint32_t                    watchdog_interval_ms;   //喂狗间隔
        int                         watchdog_gpio_pin;      //喂狗引脚号                   


    };

    //=======================================
    //线程配置
    //=======================================
    struct ThreadConfig
    {
        int         policy;     //SCHED_FIFO/SCHED_RR/SCHED_OTHER
        int         priority;   //1-99
        std::string name;       //线程名字，调度用

    };

    }  //namespace meimei end
