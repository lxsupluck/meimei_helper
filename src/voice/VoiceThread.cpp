#include "VoiceThread.h"
#include "Config.h"
#include <iostream>
#include <functional>

namespace meimei {
namespace voice {

bool VoiceThread::init(VoiceRecorder::Config rec_cfg, const std::string& audio_device)
{
    output_dir_ = rec_cfg.output_dir;
    rec_cfg.debug_save = meimei::config::DEBUG_MODE;

    // 设置流式回调：每 100ms 音频块 → 喂入 STT
    rec_cfg.on_audio = [this](const int16_t* data, size_t samples) {
        stt_.feed_audio(data, samples);
    };

    if (!recorder_.init(rec_cfg)) {
        std::cerr << "[VoiceThread] 录音器初始化失败" << std::endl;
        return false;
    }

    std::cout << "[VoiceThread] 初始化完成（IAT 流式转写）" << std::endl;
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
    recorder_.request_stop();  // 打断阻塞的 ALSA 录音
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
        // recorder_.record() 阻塞录音
        //   - on_audio 回调实时喂入 STT WebSocket
        //   - debug_save=true 时同时保存 WAV
        std::string wav = recorder_.record();
        if (wav.empty() && !stt_.is_active()) continue;

        // 音频结束 → 通知 STT 发末帧
        stt_.end_audio();

        // 获取转写结果
        std::string text = stt_.get_result();
        if (text.empty()) {
            std::cerr << "[VoiceThread] STT 无法识别" << std::endl;
            continue;
        }

        std::cout << "[VoiceThread] 识别结果: " << text << std::endl;
    }
}

}  // namespace voice
}  // namespace meimei
