#include "CollectThread.h"
#include "Config.h"
#include <iostream>
#include <chrono>
#include <string>
#include "Logger.h"

namespace meimei{

    //如果多个从设备我想置空，然后set_device 再赋值，就一个先不调用了，直接赋值。
    //这样貌似每多一个Modbus总线就多一个线程？是否要修改？。目前就一个应该不用。
    CollectThread::CollectThread()
        : device_(config::MODBUS_DEVICE)
        , baud_(config::MODBUS_BAUD)
        , data_bits_(config::MODBUS_DATA_BITS)
        , stop_bits_(config::MODBUS_STOP_BITS)
        , parity_(config::MODBUS_PARITY)
    {}

    CollectThread::~CollectThread()
    {
        stop();
    }

    void CollectThread::set_device(const std::string& device,int baud, 
        char parity, int data_bits, int stop_bits )
    {
        device_     =   device;
        baud_       =   baud;
        parity_     =   parity;
        data_bits_  =   data_bits;
        stop_bits_  =   stop_bits;
    }

    void CollectThread::set_sensors(std::vector<Sensor> sensors)
    {
        for (auto& s:sensors)
        {
            SensorRuntime rt;
            rt.config = std::move(s);

            uint16_t min_addr = 0xFFFF, max_addr = 0;
            for (const auto& ch : rt.config.channels)
            {
                if (min_addr > ch.register_addr) min_addr = ch.register_addr;
                if (max_addr < ch.register_addr) max_addr = ch.register_addr;
            }
            rt.read_start = min_addr;
            rt.read_count = max_addr - min_addr + 1;

            sensors_.push_back(std::move(rt));
        }
    }

    bool CollectThread::start()
    {   
        if (running_)   return true;

        if (!modbus_.connect(device_, baud_, parity_, data_bits_, stop_bits_))
        {
            std::cout << "[CollectThread] 连接失败：" << modbus_.last_error() << std::endl;
            return false;
        }

        csv_.open(config::CSV_DIR, config::CSV_MAX_FILES);
        
        running_ = true;
        thread_ = std::thread(&CollectThread::run, this);

        return true;
    }

    void CollectThread::stop()
    {
        if(!running_) return;

        running_ = false;
        if(thread_.joinable())
            thread_.join();
        modbus_.disconnect();
        csv_.close();
        std::cout << "[CollectThread] 已停止" << std::endl;
    }

    bool CollectThread::is_running() const
    {
        return running_;
    }

    float CollectThread::current_temp() const
    {
        return current_temp_;
    }

    float CollectThread::current_humi() const
    {
        return current_humi_;
    }

    std::vector<float> CollectThread::sample(const SensorRuntime& rt)
    {
        std::vector<uint16_t> buf(rt.read_count);

        modbus_.set_slave(rt.config.slave_id);
        if(!modbus_.read_holding_registers(rt.config.slave_id, rt.read_start, rt.read_count, buf.data()))
        {
            std::cout << "[CollectThread] 连接失败：" << modbus_.last_error() << std::endl;
            return {};
        }

        std::vector<float> values;
        //减少扩容开销
        values.reserve(rt.config.channels.size());
        for(const auto& ch:rt.config.channels)
        {
            uint16_t raw = buf[ch.register_addr - rt.read_start];
            values.push_back( raw * ch.scale_factor + ch.offset);
        }
        return values;
    }

    void CollectThread::run()
    {
        using namespace std::chrono;

        

        // 正常输出参数（首窗结束后再对齐）
        constexpr uint32_t step_ms = config::OUTPUT_INTERVAL_MS;
        
        uint64_t next_out_ms = 0;   // 首窗结束后再赋值
        // const uint64_t first_step_ms = config::FIRST_WINDOW_SAMPLES * config::SAMPLE_INTERVAL_MS;
        constexpr uint64_t expected_samples = step_ms / config::SAMPLE_INTERVAL_MS;
        //向上取整
        constexpr uint32_t min_sample   = (expected_samples * config::SAMPLE_TOLERANCE_PERCENT + config::SAMPLE_INTERVAL_MS -1 ) / config::SAMPLE_INTERVAL_MS;
        constexpr uint32_t first_sample = config::FIREST_WINDOW_BOUNDARY_MS / config::SAMPLE_INTERVAL_MS;
        constexpr uint32_t first_min = (first_sample * config::SAMPLE_TOLERANCE_PERCENT + config::SAMPLE_INTERVAL_MS - 1) / config::SAMPLE_INTERVAL_MS;
        
        bool        first_window_done = false;
        uint32_t    first_sample_ms   = 0;

        float temp_sum = 0, humi_sum = 0;
        uint32_t temp_count = 0, humi_count = 0;
        // uint32_t temp_total = 0, humi_total = 0;
        uint64_t last_ts_ms = 0;

        const auto& rt = sensors_[0];
        const auto& ch0 = rt.config.channels[0];
        const auto& ch1 = rt.config.channels[1];


        std::cout << "[CollectThread] 采集启动， 快采样间隔 = " << config::SAMPLE_INTERVAL_MS << " ms, 输出间隔 =" 
            << step_ms << " ms， 首窗采样时间 = " << config::FIREST_WINDOW_BOUNDARY_MS << " ms。" << std::endl;

        uint64_t now_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        const uint64_t first_boundary = config::FIREST_WINDOW_BOUNDARY_MS + now_ms;
        constexpr auto sample_step = milliseconds(config::SAMPLE_INTERVAL_MS);
        auto next_sample = steady_clock::now();

        while(running_)
        {
            // 快采样
            auto values = sample(rt);

            now_ms = duration_cast<milliseconds>( system_clock::now().time_since_epoch()).count();

            if(!values.empty())
            {
                if(first_sample_ms == 0)
                    first_sample_ms = now_ms;
                
                current_temp_ = values[0]; temp_sum += values[0]; temp_count++;
                current_humi_ = values[1]; humi_sum += values[1]; humi_count++;
                last_ts_ms = now_ms;
            }

            if(!first_window_done)
            {   
                float temp_avg = 0, humi_avg = 0;
                if(now_ms >= first_boundary)
                {
                    if(temp_count == 0)
                    {
                        LOG_WARN("首窗温度掉拍： " + std::to_string(temp_count) + " / " + std::to_string(first_sample));
                    }
                    else
                    {
                        temp_avg = temp_sum / temp_count;

                        if(humi_count == 0)
                        {
                            LOG_WARN("首窗湿度掉拍： " + std::to_string(humi_count) + " / " + std::to_string(first_sample));
                        }
                        else
                        {
                            humi_avg = humi_sum / humi_count;

                            if(temp_count < first_min)
                                LOG_WARN("首窗温度掉拍： " + std::to_string(temp_count) + " / " + std::to_string(first_sample));
                            if(humi_count < first_min)
                                LOG_WARN("首窗湿度掉拍： " + std::to_string(humi_count) + " / " + std::to_string(first_sample));

                            int64_t drift = (int64_t)last_ts_ms - (int64_t)first_boundary;
                            if(drift > (int64_t)config::TIME_TOLERANCE_MS || drift < -(int64_t)config::TIME_TOLERANCE_MS)
                                LOG_WARN("首窗时间偏差：" + std::to_string(drift)+ " ms");

                            csv_.record(first_boundary, temp_avg, humi_avg);

                            if(temp_avg > ch0.alarm_high) LOG_WARN("温度过高： " + std::to_string(temp_avg) + " 摄氏度");
                            if(temp_avg < ch0.alarm_low)  LOG_WARN("温度过低： " + std::to_string(temp_avg) + " 摄氏度");
                            if(humi_avg > ch1.alarm_high) LOG_WARN("湿度过高： " + std::to_string(humi_avg) + " %RH");
                            if(humi_avg < ch1.alarm_low)  LOG_WARN("湿度过低： " + std::to_string(humi_avg) + " %RH");

                            std::cout << "[CollectThread] 首窗： T = " << temp_avg << " ℃ " << "H = " << humi_avg << "%RH"
                                << "（T 采样数量：" << temp_count << " / " << first_min << " ）" << std::endl;
                        }
                    }

                    first_window_done = true;
                    next_out_ms = (now_ms / step_ms) * step_ms + step_ms;

                    temp_sum = humi_sum = temp_count = humi_count = 0;
                }
            }
            else
            {   
                float temp_avg = 0, humi_avg = 0;
                if(now_ms >= next_out_ms)
                {
                    if(temp_count == 0)
                    {
                        LOG_WARN("温度掉拍： " + std::to_string(temp_count) + " / " + std::to_string(expected_samples));
                    }
                    else
                    {
                        temp_avg = temp_sum / temp_count;

                        if(humi_count == 0)
                        {
                            LOG_WARN("湿度掉拍： " + std::to_string(humi_count) + " / " + std::to_string(expected_samples));
                        }
                        else
                        {
                            humi_avg = humi_sum / humi_count;

                            if(temp_count < min_sample)
                                LOG_WARN("温度掉拍： " + std::to_string(temp_count) + " / " + std::to_string(expected_samples));
                            if(humi_count < min_sample)
                                LOG_WARN("湿度掉拍： " + std::to_string(humi_count) + " / " + std::to_string(expected_samples));

                            int64_t drift = (int64_t)last_ts_ms - (int64_t)next_out_ms;
                            if(drift > (int64_t)config::TIME_TOLERANCE_MS || drift < -(int64_t)config::TIME_TOLERANCE_MS)
                                LOG_WARN("时间偏差：" + std::to_string(drift)+ " ms");

                            csv_.record(next_out_ms, temp_avg, humi_avg);

                            if(temp_avg > ch0.alarm_high) LOG_WARN("温度过高： " + std::to_string(temp_avg) + " 摄氏度");
                            if(temp_avg < ch0.alarm_low)  LOG_WARN("温度过低： " + std::to_string(temp_avg) + " 摄氏度");
                            if(humi_avg > ch1.alarm_high) LOG_WARN("湿度过高： " + std::to_string(humi_avg) + " %RH");
                            if(humi_avg < ch1.alarm_low)  LOG_WARN("湿度过低： " + std::to_string(humi_avg) + " %RH");

                            std::cout << "[CollectThread] 检测结果： T = " << temp_avg << " ℃ " << "H = " << humi_avg << "%RH"
                                << "（T 采样数量：" << temp_count << " / " << min_sample << " ）" << std::endl;
                        }
                    }

                    next_out_ms = (now_ms / step_ms) * step_ms + step_ms;
                    temp_sum = humi_sum = temp_count = humi_count = 0;
                }
            }

            auto steady_now = steady_clock::now();
            auto since_ms   = duration_cast<milliseconds>(steady_now.time_since_epoch());
            next_sample = steady_clock::time_point(
                since_ms - (since_ms % sample_step) + sample_step
            );
            std::this_thread::sleep_until(next_sample);

        }


        
    }


} //namespace meimei