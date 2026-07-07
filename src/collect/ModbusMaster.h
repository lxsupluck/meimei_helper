#pragma once

#include <string>
#include <mutex>
#include <cstdint>
#include <vector>


//避免在.h文件中引入libmodbus的头文件，隔离解耦
struct _modbus; //
typedef struct _modbus modbus_t; 

namespace meimei{

    //===================================
    //  Modbus 主机封装他
    //  基于 libmodbus, 线程安全
    //===================================


    class ModbusMaster
    {
    public:
        ModbusMaster();
        //c++11析构函数默认noexcept不能出现异常。
        ~ModbusMaster() noexcept;

        //禁止拷贝，默认会禁止移动
        ModbusMaster( const ModbusMaster&) = delete;
        ModbusMaster& operator = (const ModbusMaster& ) = delete;
                //允许移动
        ModbusMaster( ModbusMaster&& Other) noexcept;
        ModbusMaster& operator = (ModbusMaster&& other) noexcept;

        bool connect( const std::string& device, int baud, char parity, 
            int data_bits, int stop_bits);
        
        void disconnect();

        bool is_connected() const;


        //设置Modbus从站地址和响应超时
        void set_slave(int slave_id);
        int get_slave() const;
        void set_response_timeout_ms(uint32_t timeout_ms);

        //读保持寄存器
        //slave_id 从站地址, addr 寄存器起始地址， count 读取数量， dest：结果数组
        bool read_holding_registers(int slave_id, int addr, int count, uint16_t* dest);
        bool read_holding_registers(int slave_id, int addr, int count, std::vector<uint16_t>& out);
        
        //读输入寄存器
        bool read_input_registers(int slave_id, int addr, int count, uint16_t* dest);
        bool read_input_registers(int slave_id, int addr, int count, std::vector<uint16_t>& out);

        //写单个寄存器
        bool write_single_register(int slave_id, int addr, uint16_t value);

        // 获取最后一次错误信息
        std::string last_error() const;

    private:
            modbus_t*   ctx_;
            int slave_id_;
            //mutable 和const 相关？
            mutable std::mutex  mtx_;
            std::string last_error_;
            bool        connected_;
            int get_slave_nolock() const;

    };


}
