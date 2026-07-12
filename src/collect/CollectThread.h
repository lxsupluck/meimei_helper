#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <cstdint>
#include <vector>

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
        void set_sensors(std::vector<Sensor> sensors);
        
        //启动停止
        bool start();
        void stop();

        //查询
        bool is_running() const;
        float current_temp() const;
        float current_humi() const;



    private:
            void run();
            std::vector<float> sample(const SensorRuntime& rt);
            
            //串口
            std::string device_;
            int         baud_;
            char        parity_;
            int         data_bits_;
            int         stop_bits_;
            
            //传感器参数
            std::vector<SensorRuntime> sensors_;
            //最新读数
            std::atomic<float>  current_temp_{0.0f};
            std::atomic<float>  current_humi_{0.0f};
            std::atomic<bool>   running_{false};
            std::thread         thread_;
            ModbusMaster        modbus_;

            CsvRecorder csv_;
    };


}