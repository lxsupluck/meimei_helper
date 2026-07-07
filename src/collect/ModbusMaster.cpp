#include "ModbusMaster.h"
#include <modbus/modbus.h>
#include <vector>

#include <cstring>
#include <stdexcept>

namespace meimei{

    // 构造函数
    ModbusMaster::ModbusMaster()
        : ctx_(nullptr)
        , connected_(false)
        , slave_id_(1)
    {}

    // 析构函数
    ModbusMaster::~ModbusMaster()
    {
        disconnect();

    }

    //移动构造，无返回值
    ModbusMaster::ModbusMaster(ModbusMaster&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.mtx_);
        ctx_ = other.ctx_;
        connected_ = other.connected_;
        slave_id_ = other.slave_id_;
        last_error_ =std::move(other.last_error_);

        other.ctx_ = nullptr;
        other.connected_ = false;

    }

    //移动赋值, 需要返回值
    ModbusMaster&ModbusMaster::operator = (ModbusMaster&& other) noexcept
    {
        if(this == & other)
            return *this;

        disconnect();
        std::lock_guard<std::mutex> lock(other.mtx_);

        ctx_ = other.ctx_;
        connected_ = other.connected_;
        slave_id_ = other.slave_id_;
        last_error_ = std::move(other.last_error_);

        other.ctx_ = nullptr;
        other.connected_ = false;

        return *this;

    }


    //连接
    bool ModbusMaster::connect(const std::string& device, 
        int baud, char parity, int data_bits, int stop_bits)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        //断开已有连接
        if (ctx_ != nullptr)
        {
            modbus_close(ctx_);
            modbus_free(ctx_);
            ctx_ = nullptr;
            connected_ = false;
        }

        // 创建 RTU 上下文
        ctx_ = modbus_new_rtu(device.c_str(), baud, parity, data_bits, stop_bits);
        if (ctx_ == nullptr )
        {
            last_error_ = "modbus_new_rtu 失败： 内存不足";
            return false;

        }

        // 建立连接
        if(modbus_connect(ctx_) == -1 )
        {
            last_error_ = std::string("modbus_connect 失败：") + modbus_strerror(errno);
            modbus_free(ctx_);
            return false; 
        }

        connected_ = true;
        last_error_.clear();
        return true;
    }

    // 断开连接
    void ModbusMaster::disconnect()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(ctx_)
        {
            modbus_close(ctx_);
            modbus_free(ctx_);
            ctx_ = nullptr;
        }

        connected_ = false;

    }

    // 检查连接
    bool ModbusMaster::is_connected() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return connected_;
    }

    void ModbusMaster::set_slave(int slave_id)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(slave_id < 0 || slave_id > 247 )
        {
            last_error_ = "slave id is out of range (0~247).";
            return;
        }

        slave_id_ = slave_id;
        if(ctx_)
        {
            modbus_set_slave(ctx_,slave_id);
        }

    }

    int ModbusMaster::get_slave () const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return slave_id_;
    }

    int ModbusMaster::get_slave_nolock() const { return slave_id_; }

    void ModbusMaster::set_response_timeout_ms(uint32_t timeout_ms)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(!ctx_) return;

        // limodbus单位是s
        uint32_t sec = timeout_ms / 1000;
        uint32_t usec = timeout_ms % 1000;
        modbus_set_response_timeout(ctx_, sec, usec);
    }


    std::string ModbusMaster::last_error() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return last_error_;

    }

    //内部统一读取错误码。 static:仅作为内部使用，const char* 不允许改变内容
    static void set_modbus_error(std::string& err_buf, modbus_t* ctx, const char *op)
    {
        int e = errno;
        err_buf = std::string(op) + " failed: " + modbus_strerror(e);

    }

    // 读保持寄存器, 0x03
    bool ModbusMaster::read_holding_registers(int slave_id, int addr
        , int count, uint16_t* dest)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if( !ctx_ || !connected_)
        {
            last_error_ = "设备未连接！！！";
            return false;
        }

        //要用to_string string(3) = "\0\0\0" , &不能出现在变量名
        if(slave_id < 0 || slave_id > 247 || count < 0 || count > 125)
        {
            std::string s_and_c = std::to_string(slave_id) + " / " + std::to_string(count);
            last_error_ = "invalid slave id / register count" + s_and_c;
            return false;
        }
        
        if(!dest)
        {
            last_error_ = "dest buff nullptr";
            return false;
        }

        modbus_set_slave(ctx_, slave_id);

        int ret = modbus_read_registers(ctx_, addr, count, dest);
        if(ret == -1)
        {
            last_error_ = std::string("读保持寄存器失败： ") + modbus_strerror(errno);
            return false;
        }
        last_error_.clear();
        return true;
    }

    bool  ModbusMaster::read_holding_registers(int slave_id, int addr, int count, std::vector<uint16_t>& out)
    {
        out.resize(count);
        //用 out.data() 转成 uint16_t* 来调用指针版。
        return read_holding_registers(slave_id, addr, count, out.data());
    }

    //读输入寄存器，指针
    bool ModbusMaster::read_input_registers(int slave_id, int addr, int count, uint16_t* dest)
    {
        std::lock_guard<std::mutex> lock(mtx_);

                if( !ctx_ || !connected_)
        {
            last_error_ = "设备未连接！！！";
            return false;
        }

        if(slave_id < 0 || slave_id > 247 || count < 0 || count > 125)
        {
            std::string s_and_c = std::to_string(slave_id) + " / " + std::to_string(count);
            last_error_ = "invalid slave id / register count" + s_and_c;
            return false;
        }
        
        if(!dest)
        {
            last_error_ = "dest buff nullptr";
            return false;
        }

        modbus_set_slave(ctx_, slave_id);
        int ret = modbus_read_input_registers(ctx_, addr, count, dest);
        if(ret == -1)
        {
            last_error_ = std::string("读输入寄存器失败： ") + modbus_strerror(errno);
            return false;

        }

        last_error_.clear();
        return true;
    }

    bool ModbusMaster::read_input_registers(int slave_id, int addr, int count, std::vector<uint16_t>& out)
    {
        out.resize(count);
        return read_input_registers(slave_id, addr, count, out.data());
    }

    // 写单个寄存器
    bool ModbusMaster::write_single_register(int slave_id, int addr, uint16_t value)
    {
        std::lock_guard<std::mutex> lock(mtx_);

                if( !ctx_ || !connected_)
        {
            last_error_ = "设备未连接！！！";
            return false;
        }

        if(slave_id < 0 || slave_id > 247 )
        {
            last_error_ = "invalid slave id" + std::to_string(slave_id);
            return false;
        }

        modbus_set_slave(ctx_, slave_id);
        int ret = modbus_write_register(ctx_, addr, value);
        if(ret == -1)
        {
            last_error_ = std::string("写单个寄存器失败： ") + modbus_strerror(errno);
            return false;
        }

        last_error_.clear();
        return true;
    }


}