#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <cstdint>
#include "CsvRecorder.h"
#include "Config.h"
#include "ModbusMaster.h"
#include "Types.h"


namespace meimei
{
    class CollectThread
    {
    public:
        CollectThread();
        ~CollectThread();

        //
        CollectThread(const CollectThread& ) = delete;
        CollectThread& operator =(const CollectThread&) = delete;

        void set_device(const std::string&device, int baud, char parity,
            int data_bits, int stop_bits );
        void set_sensor_config( int slave_id, int temp_reg, int humi_reg,
            float temp_scale, float humi_scale, uint32_t interval_ms);
        
        //启动停止
        bool start();
        void stop();

        //查询
        bool is_running() const;
        float current_temp() const;
        float current_humi() const;

    private:
            void run();
            
            //串口
            std::string device_;
            int         baud_;
            char        parity_;
            int         data_bits_;
            int         stop_bits_;
            
            //传感器参数
            int         slave_id_;
            int         temp_reg_;
            int         humi_reg_;
            float       temp_scale_;
            float       humi_scale_;
            uint32_t    interval_ms_;

            //最新读数
            std::atomic<float>  current_temp_{0.0f};
            std::atomic<float>  current_humi_{0.0f};
            std::atomic<bool> running_{false};
            std::thread thread_;
            ModbusMaster        modbus_;

            CsvRecorder csv_;
            float       temp_high_ = config::ALARM_TEMP_HIGH;
            float       temp_low_  = config::ALARM_TEMP_LOW;
            bool        first_csv_ = false;

    };


}