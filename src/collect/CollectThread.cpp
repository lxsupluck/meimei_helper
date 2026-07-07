#include "CollectThread.h"
#include "Config.h"
#include <iostream>
#include <chrono>
#include <string>
#include "Logger.h"

namespace meimei{

    CollectThread::CollectThread()
        : device_("/dev/ttyUSB0")
        , baud_(9600)
        , data_bits_(8)
        , stop_bits_(1)
        , slave_id_(1)
        , temp_reg_(1)
        , humi_reg_(0)
        , temp_scale_(0.1)
        , humi_scale_(0.1)
        , interval_ms_(2000)
        , parity_('N')
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

    void CollectThread::set_sensor_config(int slave_id, int temp_reg, int humi_reg,
        float temp_scale, float humi_scale, uint32_t interval_ms)
    {
        slave_id_       =   slave_id;
        temp_reg_       =   temp_reg;
        humi_reg_       =   humi_reg;
        temp_scale_     =   temp_scale;
        humi_scale_     =   humi_scale;
        interval_ms_    =   interval_ms;
    }

    bool CollectThread::start()
    {   
        if (running_)   return true;

        if (!modbus_.connect(device_, baud_, parity_, data_bits_, stop_bits_))
        {
            std::cout << "[CollectThread] 连接失败：" << modbus_.last_error() << std::endl;
            return false;
        }

        modbus_.set_slave(slave_id_);
        std::cout << "[CollectThread] 已连接: " << device_ 
            << " 从站id: " << slave_id_ << std::endl;

        csv_.open(config::CSV_DIR, config::CSV_INTERVAL_S, config::CSV_MAX_FILES);
        csv_.force_record(0.0f, 0.0f);
        
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

    void CollectThread::run()
    {
        std::cout << "[CollectedThread] 启动采集，间隔：" << interval_ms_ << " ms" <<std::endl;

        while(running_)
        {
            uint16_t buf[2];

            if( modbus_.read_holding_registers(slave_id_, temp_reg_, 2, buf))
            {
                float temp = buf[0] * temp_scale_;
                float humi = buf[1] * humi_scale_;

                current_temp_ = temp;
                current_humi_ = humi;

                if (!first_csv_)
                {
                    csv_.force_record(temp, humi);
                    first_csv_ = true;
                }
                else
                {
                    csv_.try_record(temp, humi);
                }

                // 告警
                if (temp > temp_high_)
                    LOG_WARN("温度过高: " + std::to_string(temp) + " ℃");
                if (temp < temp_low_)
                    LOG_WARN("温度过低: " + std::to_string(temp) + " ℃");

            

            std::cout << "[采集] 温度： " << current_temp_ << " ℃" 
                << " 湿度： "<< current_humi_ << " %RH" << std::endl;
            }
            else
            {
                std::cerr << "[CollectThread] 读取失败: " << modbus_.last_error() << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
            
        }
    }


} //namespace meimei