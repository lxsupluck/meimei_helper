#include <iostream>
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>

#include "Types.h"
#include "Config.h"
#include "CollectThread.h"
#include "Logger.h"
#include "VoiceThread.h"

// 全局指针
meimei::CollectThread*      g_collect = nullptr;
meimei::voice::VoiceThread* g_voice   = nullptr;

// 系统状态
static std::atomic<meimei::SystemState> g_state{meimei::SystemState::INIT};

void signal_handler(int /*sig*/)
{
    // 只做 async-signal-safe 的操作：设置原子标志
    // 实际的 stop/join 由主循环后处理，避免 Resource deadlock avoided
    g_state.store(meimei::SystemState::SHUTDOWN, std::memory_order_release);
}

int main()
{
    // ============================================
    // INIT 阶段
    // ============================================
    g_state.store(meimei::SystemState::INIT);

    meimei::Logger::Instance().init(meimei::config::LOG_FILE_PATH,
                                     meimei::config::LOG_LEVEL);
    LOG_INFO("莓莓助手 启动成功！！！");
    std::cout << "莓莓助手 启动成功！！！" << std::endl;

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // ── 配置采集线程 ──
    meimei::CollectThread collect;
    collect.set_device(meimei::config::MODBUS_DEVICE, meimei::config::MODBUS_BAUD,
                       meimei::config::MODBUS_PARITY, meimei::config::MODBUS_DATA_BITS,
                       meimei::config::MODBUS_STOP_BITS);

    meimei::Sensor th_sensor;
    th_sensor.id       = 1;
    th_sensor.name     = "TH Sensor";
    th_sensor.slave_id = meimei::config::THSENS_SLAVE_ID;
    th_sensor.channels = {
        { static_cast<uint16_t>(meimei::config::SENSOR_TEMP_REG),
          meimei::config::SENSOR_TEMP_SCALE, meimei::config::SENSOR_TEMP_OFFSET,
          meimei::config::TEMP_UNIT,
          meimei::config::ALARM_TEMP_HIGH, meimei::config::ALARM_TEMP_LOW },
        { static_cast<uint16_t>(meimei::config::SENSOR_HUMI_REG),
          meimei::config::SENSOR_HUMI_SCALE, meimei::config::SENSOR_HUMI_OFFSET,
          meimei::config::HUMI_UNIT,
          meimei::config::ALARM_HUMI_HIGH, meimei::config::ALARM_HUMI_LOW }
    };

    collect.set_sensors({std::move(th_sensor)});
    g_collect = &collect;

    if (!collect.start()) {
        std::cerr << "采集启动失败" << std::endl;
        return 1;
    }

    // ── 配置语音线程 ──
    meimei::voice::VoiceThread voice;
    meimei::voice::VoiceRecorder::Config rec_cfg;
    rec_cfg.device            = meimei::config::AUDIO_DEVICE;
    rec_cfg.sample_rate       = meimei::config::SAMPLE_RATE;
    rec_cfg.silence_timeout_ms = meimei::config::SILENCE_TIMEOUT_MS;
    rec_cfg.max_record_ms     = meimei::config::MAX_RECORD_MS;
    rec_cfg.speech_threshold  = meimei::config::SPEECH_THRESHOLD;
    rec_cfg.output_dir        = meimei::config::OUTPUT_DIR;

    if (!voice.init(rec_cfg, meimei::config::AUDIO_DEVICE)) {
        std::cerr << "语音初始化失败" << std::endl;
    } else {
        g_voice = &voice;
        voice.start();
    }

    // ============================================
    // RUNNING / DEBUG 阶段
    // ============================================
    g_state.store(meimei::SystemState::DEBUG);

    while (collect.is_running() &&
           g_state.load(std::memory_order_acquire) != meimei::SystemState::SHUTDOWN) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        meimei::Logger::Instance().tick();
    }

    if (g_state.load() == meimei::SystemState::SHUTDOWN) {
        std::cout << "\n收到退出信号，正在关闭..." << std::endl;
    }

    // ============================================
    // SHUTDOWN 阶段
    // ============================================
    g_state.store(meimei::SystemState::SHUTDOWN);

    voice.stop();
    collect.stop();

    std::cout << "莓莓助手 正常退出" << std::endl;
    return 0;
}
