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

    bool CsvRecorder::open(const std::string& dir, int max_files)
    {
        dir_       = dir;
        max_files_ = max_files;

        // 创建目录
        mkdir(dir_.c_str(), 0755);

        // 清理旧文件
        rotate();

        // 延迟到首次 record() 再创建文件（此时才确定日期）
        current_day_ = 0;

        std::cout << "[CsvRecorder] 已就绪，目录： " << dir_ << std::endl;
        return true;
    }

    void CsvRecorder::close()
    {
        if (file_.is_open())
            file_.close();
    }

    bool CsvRecorder::is_open() const
    {
        return file_.is_open();
    }

    void CsvRecorder::record(uint64_t ts_ms, float temp, float humi)
    {
        uint64_t day = ts_ms / 86400000ULL;

        // 跨天 → 切换文件
        if (day != current_day_)
        {
            if (file_.is_open())
                file_.close();

            // day → 日期字符串
            time_t day_sec = static_cast<time_t>(day * 86400);
            struct tm tm_buf;
            localtime_r(&day_sec, &tm_buf);
            char name[64];
            std::strftime(name, sizeof(name), "data_%Y%m%d.csv", &tm_buf);

            std::string path = dir_ + "/" + name;
            file_.open(path, std::ios::out | std::ios::app);
            if (!file_.is_open())
            {
                std::cerr << "[CsvRecorder] 无法打开文件: " << path << std::endl;
                return;
            }

            // 空文件才写表头
            file_.seekp(0, std::ios::end);
            if (file_.tellp() == 0)
            {
                file_ << "timestamp,temperature(℃),humidity(%RH)" << std::endl;
            }

            current_day_ = day;
            rotate();

            std::cout << "[CsvRecorder] 切换至： " << path << std::endl;
        }

        write_row(ts_ms, temp, humi);
    }

    void CsvRecorder::write_row(uint64_t ts_ms, float temp, float humi)
    {
        if (!file_.is_open())
            return;

        time_t sec = static_cast<time_t>(ts_ms / 1000);
        int    ms  = static_cast<int>(ts_ms % 1000);
        struct tm tm_buf;
        localtime_r(&sec, &tm_buf);
        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_buf);

        file_ << time_buf << "." << std::setfill('0') << std::setw(3) << ms << ","
              << std::fixed << std::setprecision(1)
              << temp << ","
              << humi << std::endl;
    }

    void CsvRecorder::rotate()
    {
        std::string cmd = "ls -1 " + dir_ + "/data_*.csv 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return;

        std::vector<std::string> files;
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe))
        {
            std::string f(buf);
            while (!f.empty() && (f.back() == '\n' || f.back() == '\r'))
                f.pop_back();
            if (!f.empty())
                files.push_back(f);
        }
        pclose(pipe);

        std::sort(files.begin(), files.end());

        int to_delete = static_cast<int>(files.size()) - max_files_;
        for (int i = 0; i < to_delete; ++i)
        {
            std::remove(files[i].c_str());
            std::cout << "[CsvRecorder] 删除旧文件： " << files[i] << std::endl;
        }
    }

} //namespace meimei end