#include "CsvRecorder.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace meimei
{

    CsvRecorder::~CsvRecorder()
    {
        close();
    }

    bool CsvRecorder::open(const std::string& dir, uint32_t interval_s, int max_files)
    {
        dir_        = dir;
        interval_s_ = interval_s;
        max_files_   = max_files;
        
        // 创建目录
        mkdir(dir_.c_str(), 0755);

        // 清理旧文件
        rotate();
        
        //生成文件名： data_20260707_143000.csv
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        auto tm  = *std::localtime(&t);
        char name[64];
        std::strftime(name, sizeof(name), "data_%Y%m%d_%H%M%S.csv", &tm);

        std::string path = dir_+ "/" + name;
        file_.open(path, std::ios::out | std::ios::trunc);
        if(!file_.is_open())
        {
            std::cerr << "[CsvRecorder] 无法创建文件: " << path << std::endl;
            return false;
        }

        //csv 表头
        file_ << "timestamp,temperature(℃),humidity(%RH)";

        first_done_ = false;
        next_write_ms_ = 0;

        std::cout << "[CsvRecorder] 已创建： " << path << std::endl;
        return true;    

    }

    void CsvRecorder::close()
    {
        if(file_.is_open())
            file_.close();
    }

    bool CsvRecorder::is_open() const
    {
        return file_.is_open();
    }

    void CsvRecorder::force_record(float temp, float humi)
    {
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>
        (
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        write_row(now_ms, temp, humi);

        if(!first_done_)
        {
            first_done_ = true;
            next_write_ms_ = next_aligned_ms(now_ms);
        }
    }

    bool CsvRecorder::try_record(float temp, float humi)
    {

        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>
        (
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        //感觉容易出bug
        if(!first_done_)
            return false;

        if(now_ms >= next_write_ms_)
        {
            write_row(now_ms, temp, humi);
            next_write_ms_ = next_aligned_ms(now_ms);
            return true;
        }

        return false;

    }

    // ============================================================
    // 计算下一个壁钟对齐时刻
    // 以 Unix 纪元(1970-01-01 00:00:00)为基准，
    // 每 interval_s_ 秒对齐一次
    // 例: interval_s_=60  → 下个 XX:XX:00.000
    //     interval_s_=600 → 下个 XX:00:00 / XX:10:00 / ...
    // ============================================================

    uint64_t CsvRecorder::next_aligned_ms(uint64_t now_ms)
    {
        uint64_t step_ms = static_cast<uint64_t> (interval_s_) * 1000;
        return ((now_ms / step_ms) + 1 ) * step_ms;
    }

    void CsvRecorder::write_row(uint64_t ts_ms, float temp, float humi)
    {
        if (!file_.is_open())
            return;

        // 毫秒时间戳 → 日期时间字符串
        time_t sec = static_cast<time_t>(ts_ms / 1000);
        int    ms  = static_cast<int>(ts_ms % 1000);
        struct tm* tm = std::localtime(&sec);
        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);

        file_ << time_buf << "." << ms << ","
              << std::fixed << std::setprecision(1)
              << temp << ","
              << humi << std::endl;
    }

    void CsvRecorder::rotate()
    {

        //用popen 列出目录中的data_*.csv
        std::string cmd = "ls -1 " + dir_ + "/data_*.csv 2>/dev/null";
        FILE* pipe =popen(cmd.c_str(), "r");
        if(!pipe) return;

        std::vector<std::string> files;
        char buf[512];
        while(fgets(buf, sizeof(buf), pipe))
        {

            std::string f(buf);
            while(!f.empty() && (f.back() == '\n' || f.back() == '\r'))
            {
                f.pop_back();

            }

            if (!f.empty())
                files.push_back(f);

        }
        pclose(pipe);

        std::sort(files.begin(), files.end());

        int to_delete = static_cast<int> (files.size() ) - max_files_;
        for(int i = 0; i < to_delete; ++i)
        {
            std::remove(files[i].c_str());
            std::cout <<"[CsvRecorder] 删除旧文件： " << files[i] << std::endl;

        }

    }



} //namespace meimei end
