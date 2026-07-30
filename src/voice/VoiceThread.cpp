#include "VoiceThread.h"
#include "Config.h"
#include <iostream>

namespace meimei {
namespace voice {

bool VoiceThread::init(VoiceRecorder::Config rec_cfg, const std::string& audio_device)
{
    output_dir_ = rec_cfg.output_dir;

    if (!recorder_.init(rec_cfg)) {
        std::cerr << "[VoiceThread] 录音器初始化失败" << std::endl;
        return false;
    }

    std::cout << "[VoiceThread] 初始化完成" << std::endl;
    return true;
}

bool VoiceThread::start()
{
    if (running_) return true;

    running_ = true;
    thread_ = std::thread(&VoiceThread::run, this);
    return true;
}

void VoiceThread::stop()
{
    if (!running_) return;
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

VoiceThread::~VoiceThread()
{
    stop();
}

bool VoiceThread::is_running() const
{
    return running_;
}

void VoiceThread::run()
{
    while (running_) {
        std::string wav = recorder_.record(output_dir_);
        if (wav.empty()) continue;

        std::string text = stt_.transcribe(wav);
        if (text.empty()) {
            std::cerr << "[VoiceThread] STT 无法识别" << std::endl;
            continue;
        }

        std::cout << "[VoiceThread] 识别结果: " << text << std::endl;
    }
}

}  // namespace voice
}  // namespace meimei
