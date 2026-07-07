#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <fstream>
#include <atomic>
#include "Types.h"

namespace meimei
{   
    class Logger
    {
    public:
            static Logger& Instance();

        // 初始化
        void init(const std::string& file_path, LogLevel level = LogLevel::INFO);
        void shutdown();

        // 日志输出
        void log(LogLevel level, const char* file, int line, 
            const char* thread_name, const std::string& msg);
        
        // 定时刷盘
        void tick();

        //设置过滤级别
        void set_level(LogLevel level);

    private:
        Logger() =  default;
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        struct LogEntry
        {
            LogLevel    level;
            std::string text;
        };

        static const char* level_str(LogLevel level);


        std::ofstream file_;
        std::queue<LogEntry> queue_;
        std::mutex mutex_;
        std::atomic<LogLevel> level_{LogLevel::INFO};
        std::atomic<bool> running_{false};

        static constexpr size_t kMaxQueueSize = 1000;
    };

    // 便捷宏
    #define LOG_DEBUG(msg) meimei::Logger::Instance().log(meimei::LogLevel::DEBUG, __FILE__, __LINE__, "", msg)
    #define LOG_INFO(msg) meimei::Logger::Instance().log(meimei::LogLevel::INFO, __FILE__, __LINE__, "", msg)
    #define LOG_WARN(msg) meimei::Logger::Instance().log(meimei::LogLevel::WARN, __FILE__, __LINE__, "", msg)
    #define LOG_ERROR(msg) meimei::Logger::Instance().log(meimei::LogLevel::ERROR, __FILE__, __LINE__, "", msg)
    #define LOG_FATAL(msg) meimei::Logger::Instance().log(meimei::LogLevel::FATAL, __FILE__, __LINE__, "", msg)



} //meimei namespace end