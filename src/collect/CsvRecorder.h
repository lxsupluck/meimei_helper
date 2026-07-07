#pragma once

#include <string>
#include <fstream>
#include <cstdint>

namespace meimei{

    // ============================================================
    // CSV 数据记录器
    // - 首次数据立即写入
    // - 后续按壁钟整点对齐间隔写入(0分0秒为基准)
    // - 每次启动创建新文件，保留最近 N 个文件
    // ============================================================

    class CsvRecorder
    {
    public:
        CsvRecorder() = default;
        ~CsvRecorder();

        bool open(const std::string& dir, uint32_t interval_s, int max_files = 30);
        void close();
        bool is_open() const;
        //记录一次数据，内部判断是否读写该盘
        bool try_record(float temp, float humi);
        //首次记录
        void force_record(float temp, float humi);

    private:
        void rotate();
        uint64_t next_aligned_ms(uint64_t now_ms);
        void write_row(uint64_t ts_ms, float temp, float humi);

        std::ofstream   file_;
        std::string     dir_;
        uint32_t        interval_s_ = 60;
        int             max_files_ = 30;
        uint64_t        next_write_ms_ = 0;
        bool            first_done_ = false;

    };


} // namespace meimei end