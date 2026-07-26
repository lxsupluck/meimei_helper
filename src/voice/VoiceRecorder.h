#pragma once

#include <string>
#include <cstdint>
#include <alsa/asoundlib.h>

namespace meimei {
namespace voice {

// ========================================
// ALSA 录音 + 能量 VAD
// 用法:
//   VoiceRecorder rec;
//   rec.init({...});
//   std::string wav = rec.record("/tmp");  // 阻塞录音，返回 WAV 路径
// ========================================
class VoiceRecorder
{
public:
    struct Config
    {
        std::string device            = "default";
        unsigned int sample_rate       = 16000;        // 16kHz
        uint32_t    silence_timeout_ms = 1500;         // 静音 1.5s 判定说完
        uint32_t    max_record_ms      = 20000;        // 最长录 20s 兜底
        double      speech_threshold   = 300.0;        // RMS 能量阈值
    };

    VoiceRecorder() = default;
    ~VoiceRecorder();

    VoiceRecorder(const VoiceRecorder&) = delete;
    VoiceRecorder& operator=(const VoiceRecorder&) = delete;

    bool init(const Config& cfg);

    // 阻塞录音，返回 WAV 文件路径，失败返回空字符串
    std::string record(const std::string& output_dir);

private:
    static void write_wav_header(FILE* fp, uint32_t data_bytes,
                                 unsigned int sample_rate);
    static double rms_energy(const int16_t* buf, size_t n);

    Config      cfg_;
    snd_pcm_t*  pcm_ = nullptr;
    bool        ok_  = false;
};

}  // namespace voice
}  // namespace meimei
