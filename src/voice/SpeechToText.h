#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <openssl/ssl.h>

namespace meimei {
namespace voice {

// ========================================
// 讯飞 IAT WebSocket 实时语音转写
//
// 用法:
//   stt.feed_audio(data, samples);  // 每 100ms 调一次
//   stt.end_audio();                // 录音结束
//   text = stt.get_result();        // 获取转写文本
// ========================================
class SpeechToText {
public:
    SpeechToText();
    ~SpeechToText();

    SpeechToText(const SpeechToText&) = delete;
    SpeechToText& operator=(const SpeechToText&) = delete;

    // 馈入音频 (PCM 16kHz mono S16_LE)
    // 首次调用自动建 WebSocket 连接 + 发首帧
    void feed_audio(const int16_t* data, size_t samples);

    // 发送末帧，结束音频流
    void end_audio();

    // 获取转写结果（end_audio 之后调用）
    std::string get_result();

    // 是否有活跃会话
    bool is_active() const { return active_; }

private:
    // ── WebSocket + TLS 底层 ──
    bool ws_connect();
    void ws_close();
    bool ws_send_text(const std::string& msg);
    std::string ws_recv_text(int timeout_ms = 500);
    bool ws_wait_readable(int timeout_ms);

    // ── IAT 协议 ──
    std::string make_auth_url() const;
    std::string make_frame(int status, const std::string& audio_b64) const;

    // ── TLS ──
    bool tls_handshake(const char* host);
    void tls_cleanup();

    int     sock_fd_  = -1;
    SSL*    ssl_      = nullptr;
    SSL_CTX* ssl_ctx_ = nullptr;

    std::string result_;
    bool        active_        = false;
    bool        first_frame_sent_ = false;
    bool        connect_failed_   = false;  // 本轮连接失败，不再重试
};

}  // namespace voice
}  // namespace meimei
