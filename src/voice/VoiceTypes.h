#pragma once

#include <cstring>
#include <cstdint>

namespace meimei
{

    namespace voice
    {
        enum class CmdType : uint8_t
        {
            None,

            QUERY_TEMP, QUERY_HUMI, QUERY_TIME, QUERY_STATUS,

            SET_SAMPLE_INTERVAL, SET_OUTPUT_INTERVAL,

            START_COLLECT, STOP_COLLECT, SHUTDOWN, REBOOT,

            UNKNOW,
        };

        struct SpeakTemplate{
            std::string text;
        };


        SpeakTemplate build_replay(const VoiceCommand& cmd, float temp, float humi);

        //异常情况回复方案.
        SpeakTemplate build_unknow_replay();
        SpeakTemplate build_offline_reply();
        SpealTemplate build_error_replay(const st::string& reason);


    } // namespace meimei::voice end;

}// namespace meimei end;