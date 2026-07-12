#pragma once

#include <string>
#include <fstream>
#include <cstdint>

namespace meimei{

    // ============================================================
    // CSV 数据记录器（被动写入 + 按天分文件）
    // - 调用方传入时间戳，CsvRecorder 只负责写盘和轮转
    // - 文件名 data_YYYYMMDD.csv，跨天自动切换
    // - 保留最近 N 个文件
    // ============================================================

    class CsvRecorder
    {
    public:
        CsvRecorder() = default;
        ~CsvRecorder();

        bool open(const std::string& dir, int max_files = 30);
        void close();
        bool is_open() const;

        // 写一条记录（ts_ms 为 epoch 毫秒时间戳）
        void record(uint64_t ts_ms, float temp, float humi);

    private:
        void rotate();
        void write_row(uint64_t ts_ms, float temp, float humi);

        std::ofstream   file_;
        std::string     dir_;
        int             max_files_ = 30;
        uint64_t        current_day_ = 0;
    };


} // namespace meimei end