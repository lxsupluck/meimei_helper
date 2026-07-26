#pragma once

#include <string>

namespace meimei {
namespace voice {

// ========================================
// 语音转文字 — 科大讯飞 HTTP API
// ========================================
class SpeechToText {
public:
    SpeechToText() = default;
    ~SpeechToText() = default;

    std::string transcribe(const std::string& wav_path);
};

}  // namespace voice
}  // namespace meimei
