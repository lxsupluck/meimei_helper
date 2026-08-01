#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <atomic>
#include <alsa/asoundlib.h>

namespace meimei {
namespace voice {

// ========================================
// ALSA 录音 + 能量 VAD
//
// 纯文件模式（on_audio == nullptr）:
//   rec.record() → WAV 路径
//
// 流式模式（on_audio 设置）:
//   每收到 100ms 音频块都调 on_audio
//   debug_save==true 时额外存 WAV
//   返回空串
// ========================================
class VoiceRecorder
{
public:
    // 音频块回调：data + 采样数
    using AudioCallback = std::function<void(const int16_t* data, size_t samples)>;

    struct Config
    {
        std::string  device             = "default";
        unsigned int sample_rate        = 16000;
        uint32_t     silence_timeout_ms = 1500;
        uint32_t     max_record_ms      = 20000;
        double       speech_threshold   = 300.0;
        std::string  output_dir         = "/home/lx/helper/voice_temp/";

        AudioCallback on_audio   = nullptr;  // 流式回调，空则纯文件模式
        bool          debug_save = false;    // 流式模式下是否同时存 WAV
    };

    VoiceRecorder() = default;
    ~VoiceRecorder();

    VoiceRecorder(const VoiceRecorder&) = delete;
    VoiceRecorder& operator=(const VoiceRecorder&) = delete;

    bool init(const Config& cfg);

    // 阻塞录音
    // 纯文件模式：返回 WAV 路径
    // 流式模式：  返回空串
    std::string record();

    // 请求中断正在进行的录音（由其他线程调用）
    void request_stop();

private:
    static void write_wav_header(FILE* fp, uint32_t data_bytes,
                                 unsigned int sample_rate);
    static double rms_energy(const int16_t* buf, size_t n);

    Config      cfg_;
    snd_pcm_t*  pcm_ = nullptr;
    bool        ok_  = false;
    std::atomic<bool> stop_req_{false};
};

}  // namespace voice
}  // namespace meimei
