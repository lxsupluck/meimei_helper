#pragma once

#include <thread>
#include <atomic>
#include <string>

#include "VoiceRecorder.h"
#include "SpeechToText.h"

namespace meimei {
namespace voice {

class VoiceThread
{
public:
    VoiceThread() = default;
    ~VoiceThread();

    VoiceThread& operator=(const VoiceThread&) = delete;
    VoiceThread(const VoiceThread&) = delete;

    // 初始化录音器，output_dir 为 WAV 保存目录
    bool init(VoiceRecorder::Config rec_cfg, const std::string& audio_device);

    bool start();
    void stop();
    bool is_running() const;

private:
    void run();

    VoiceRecorder   recorder_;
    SpeechToText    stt_;

    std::string         output_dir_;
    std::atomic<bool>   running_{false};
    std::thread         thread_;
};

}  // namespace voice
}  // namespace meimei
