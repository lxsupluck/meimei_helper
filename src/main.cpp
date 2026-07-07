#include <iostream>
#include "Types.h"
#include "Config.h"
#include "CollectThread.h"
#include "Logger.h"
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>

//全局指针
meimei::CollectThread* g_collect = nullptr;

void signal_handler(int sig)
{
    std::cout << "\n收到信号 " << sig << "正在退出..." << std::endl;
    if (g_collect) g_collect -> stop();
}

int main()
{
    meimei::Logger::Instance().init(meimei::config::LOG_FILE_PATH, meimei::config::LOG_LEVEL);
    LOG_INFO("莓莓助手 启动成功！！！");


    std::cout << "莓莓助手 启动成功！！！" << std::endl;

    //注册信号
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    //配置采集线程
    meimei::CollectThread collect;

    collect.set_device(meimei::config::MODBUS_DEVICE, meimei::config::MODBUS_BAUD,
                       meimei::config::MODBUS_PARITY, meimei::config::MODBUS_DATA_BITS,
                       meimei::config::MODBUS_STOP_BITS);

    collect.set_sensor_config(meimei::config::SENSOR_SLAVE_ID,
                              meimei::config::SENSOR_TEMP_REG,
                              meimei::config::SENSOR_HUMI_REG,
                              meimei::config::SENSOR_TEMP_SCALE,
                              meimei::config::SENSOR_HUMI_SCALE,
                              meimei::config::SENSOR_INTERVAL_MS);

    g_collect = &collect;

    if(!collect.start())
    {
        std::cerr << "采集启动失败" << std::endl;
        return 1;
    }
    int sleep_counts = 0;
    while(collect.is_running() && (sleep_counts < 10))
    {
        std::this_thread::sleep_for(std::chrono::seconds(6));
         meimei::Logger::Instance().tick();   // 定期刷盘
        sleep_counts++;
    }
 
    std::cout << "莓莓助手 正常退出" << std::endl;
    return 0;
}