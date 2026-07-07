#include <iostream>
#include "common/Types.h"
#include "collect/CollectThread.h"
#include "logger/Logger.h"
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
    meimei::Logger::Instance().init("./meimei.log", meimei::LogLevel::DEBUG);
    LOG_INFO("莓莓助手 启动成功！！！");


    std::cout << "莓莓助手 启动成功！！！" << std::endl;

    //注册信号
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    //配置采集线程
    meimei::CollectThread collect;

    collect.set_device("/dev/ttyUSB0", 9600, 'N', 8, 1);

    collect.set_sensor_config(1, 0, 1, 0.1f, 0.1f, 2000);

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