#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>

namespace meimei
{
    Logger& Logger::Instance()
    {
        // c++11 保证局部静态变量的线程安全初始化
        static Logger inst;
        return inst;  
    }

    void Logger::init(const std::string& file_path, LogLevel level)
    {
        if(running_) return;

        level_ = level;

        file_.open(file_path, std::ios::app);
        if(!file_.is_open())
        {
            std::cerr <<"[Logger] 无法打开日志文件：" << file_path << std::endl;
            return;
        }

        running_ = true;

        // 写入启动标记
        log(LogLevel::INFO, __FILE__, __LINE__, "main", "===== logger 启动 ===== ");
    }
    void Logger::shutdown()
    {
        if(!running_) return;

        log(LogLevel::INFO, __FILE__, __LINE__, "main", "===== Logger 关闭 ===== ");
        tick(); // 刷盘

        running_ = false;
        if (file_.is_open())
        {
            file_.close();
        }

    }

    Logger::~Logger()
    {
        shutdown();

    }

    void Logger::set_level(LogLevel level)
    {
        level_ = level;
    }
    
    const char* Logger::level_str(LogLevel level)
    {
        switch(level)
        {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default:              return "UNKNOWN";
        }
        return "???";
    }

    void Logger::log(LogLevel level, const char* file, int line,
        const char* thread_name, const std::string& msg)
    {
        if (!running_) return;
        if (level < level_) return;

        //格式化 [时间] [级别] [线程] 消息 (文件：行)
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast
            <std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", 
            std::localtime(&t));
        
        std::ostringstream oss;
        oss << "[" << time_buf << "." << ms.count() <<"]" 
            << "[" << level_str(level) << "]";
        
        if(thread_name[0] != '\0')
            oss << "[" << thread_name << "]";

        // 取文件名
        const char* short_file = file;
        const char* p = file;
        while(*p) {if (*p == '/') short_file = p + 1; ++p;}

        oss << msg << "(" << short_file << ":" << line << ")";

        //入队
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.size() >= kMaxQueueSize)
                queue_.pop();

            queue_.push({level, oss.str()});

        }
    }

    void Logger::tick()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        while(!queue_.empty())
        {
            const auto& entry = queue_.front();
            if (file_.is_open())
            {
                file_ << entry.text << std::endl;

            }
            queue_.pop();
        }

        if(file_.is_open())
            file_.flush();
    }



} //namespace meimei end