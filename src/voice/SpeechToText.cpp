#include "SpeechToText.h"
#include "Config.h"

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <ctime>
#include <random>
#include <sstream>
#include <iostream>
#include <chrono>
#include <map>

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/err.h>

namespace meimei {
namespace voice {

// ═══════════════════════════════════════════
// 工具函数
// ═══════════════════════════════════════════

static std::string base64_encode(const std::string& in)
{
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out += tbl[(val >> bits) & 0x3F];
            bits -= 6;
        }
    }
    if (bits > -6)
        out += tbl[((val << 8) >> (bits + 8)) & 0x3F];
    while (out.size() % 4)
        out += '=';
    return out;
}

static std::string md5_hex(const std::string& in)
{
    unsigned char d[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(in.c_str()), in.size(), d);
    char hex[33];
    for (int i = 0; i < 16; ++i)
        std::sprintf(hex + i * 2, "%02x", d[i]);
    return std::string(hex);
}

static std::string base64_decode(const std::string& in)
{
    static const unsigned char dec[256] = {
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
        52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
        64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    };
    std::string out;
    int val = 0, bits = -8;
    for (unsigned char c : in) {
        unsigned char v = dec[c];
        if (v > 63) continue;
        val = (val << 6) + v;
        bits += 6;
        if (bits >= 0) {
            out += static_cast<char>((val >> bits) & 0xFF);
            bits -= 8;
        }
    }
    return out;
}

// 生成 n 个随机字节（用于 WebSocket key / mask）
static std::string random_bytes(size_t n)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    std::string out(n, '\0');
    for (size_t i = 0; i < n; ++i)
        out[i] = static_cast<char>(dis(gen));
    return out;
}

// ═══════════════════════════════════════════
// IAT 协议 — HMAC-SHA256 签名鉴权
// ===========================================
// 签名原文（后两行必须顶格！）:
//   host: <host>
//   date: <RFC1123>
//   GET /v2/iat HTTP/1.1
// 签名: Base64(HmacSHA256(APISecret, 签名原文))
// URL:  ?authorization=<base64>&date=<rfc1123>&host=<host>
// ═══════════════════════════════════════════

// RFC1123 格式 UTC 时间: "Thu, 01 Aug 2024 12:00:00 GMT"
static std::string rfc1123_time()
{
    time_t t = time(nullptr);
    struct tm tm_gmt;
    gmtime_r(&t, &tm_gmt);
    static const char* months[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    static const char* days[] = {
        "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
    };
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s, %02d %s %d %02d:%02d:%02d GMT",
        days[tm_gmt.tm_wday], tm_gmt.tm_mday, months[tm_gmt.tm_mon],
        tm_gmt.tm_year + 1900, tm_gmt.tm_hour, tm_gmt.tm_min, tm_gmt.tm_sec);
    return buf;
}

static std::string sha256_hex(const std::string& in)
{
    unsigned char d[32];
    SHA256(reinterpret_cast<const unsigned char*>(in.c_str()), in.size(), d);
    char hex[65];
    for (int i = 0; i < 32; ++i)
        std::sprintf(hex + i * 2, "%02x", d[i]);
    return std::string(hex);
}

std::string SpeechToText::make_auth_url() const
{
    using config::STT_API_KEY;
    using config::STT_API_SECRET;
    using config::STT_IAT_URL;

    std::string apikey    = STT_API_KEY;
    std::string apisecret = STT_API_SECRET;

    // 解析 host + path
    std::string url = STT_IAT_URL;  // wss://ws-api.xfyun.cn/v2/iat
    auto scheme_end = url.find("://");
    std::string rest = (scheme_end != std::string::npos)
        ? url.substr(scheme_end + 3) : url;
    auto slash = rest.find('/');
    std::string host = (slash != std::string::npos)
        ? rest.substr(0, slash) : rest;
    std::string path = (slash != std::string::npos)
        ? rest.substr(slash) : "/v2/iat";

    // RFC1123 日期
    std::string date = rfc1123_time();

    // ── 签名原文（后两行必须顶格，不能有前导空格！）──
    std::string sign_raw =
        "host: " + host + "\n"
        "date: " + date + "\n"
        "GET " + path + " HTTP/1.1";

    // ── HMAC-SHA256 签名 ──
    // APISecret 是控制台原始密钥，直接用作 HMAC key
    std::string real_secret = apisecret;

    unsigned char hmac_out[EVP_MAX_MD_SIZE];
    unsigned int hmac_len = 0;
    HMAC(EVP_sha256(),
         real_secret.data(), static_cast<int>(real_secret.size()),
         reinterpret_cast<const unsigned char*>(sign_raw.data()), sign_raw.size(),
         hmac_out, &hmac_len);
    std::string signature = base64_encode(
        std::string(reinterpret_cast<char*>(hmac_out), hmac_len));

    // ── 构建 Authorization ──
    std::string auth_origin =
        "api_key=\"" + apikey + "\", "
        "algorithm=\"hmac-sha256\", "
        "headers=\"host date request-line\", "
        "signature=\"" + signature + "\"";

    std::string authorization = base64_encode(auth_origin);

    // URL 编码（date 含空格和逗号，authorization 含 base64 的 +/=）
    auto url_encode = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size() * 3);
        for (unsigned char c : s) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                c == '.' || c == '~') {
                out += static_cast<char>(c);
            } else {
                out += '%';
                out += "0123456789ABCDEF"[c >> 4];
                out += "0123456789ABCDEF"[c & 0x0F];
            }
        }
        return out;
    };

    // ── 拼接 URL query（参数需 URL 编码）──
    std::string query = path
        + "?authorization=" + url_encode(authorization)
        + "&date=" + url_encode(date)
        + "&host=" + host;

    std::cerr << "[STT IAT] host=" << host
              << " date=" << date
              << " sign_raw=[" << sign_raw << "]"
              << " signature=" << signature
              << " auth_len=" << authorization.size() << std::endl;

    return query;
}

// 构建 IAT 数据帧 JSON
// status: 0=首帧, 1=中间帧, 2=末帧
std::string SpeechToText::make_frame(int status, const std::string& audio_b64) const
{
    using config::STT_APP_ID;
    std::string appid = STT_APP_ID;

    return
        "{\"common\":{\"app_id\":\"" + appid + "\"},"
        "\"business\":{"
            "\"language\":\"zh_cn\","
            "\"domain\":\"iat\","
            "\"accent\":\"mandarin\","
            "\"vad_eos\":10000,"
            "\"dwa\":\"wpgs\""
        "},"
        "\"data\":{"
            "\"status\":" + std::to_string(status) + ","
            "\"format\":\"audio/L16;rate=16000\","
            "\"encoding\":\"raw\","
            "\"audio\":\"" + audio_b64 + "\""
        "}}";
}

// ═══════════════════════════════════════════
// WebSocket 底层
// ═══════════════════════════════════════════

bool SpeechToText::ws_connect()
{
    using config::STT_IAT_URL;

    // ── 解析 host + port ──
    std::string url = STT_IAT_URL;  // wss://iat-api.xfyun.cn/v2/iat
    std::string scheme, host, path;
    int port = 443;

    auto scheme_end = url.find("://");
    if (scheme_end != std::string::npos) {
        scheme = url.substr(0, scheme_end);
        url = url.substr(scheme_end + 3);
    }

    auto path_start = url.find('/');
    if (path_start != std::string::npos) {
        host = url.substr(0, path_start);
        path = url.substr(path_start);
    } else {
        host = url;
        path = "/";
    }

    auto colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        port = std::stoi(host.substr(colon_pos + 1));
        host = host.substr(0, colon_pos);
    }

    std::cerr << "[STT WS] connecting to " << host << ":" << port << path << std::endl;

    // ── DNS resolve ──
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string port_str = std::to_string(port);

    int rc = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (rc != 0) {
        std::cerr << "[STT WS] DNS fail: " << gai_strerror(rc) << std::endl;
        return false;
    }

    sock_fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd_ < 0) {
        std::cerr << "[STT WS] socket fail" << std::endl;
        freeaddrinfo(res);
        return false;
    }

    // ── connect 超时 5s ──
    int flags = fcntl(sock_fd_, F_GETFL, 0);
    fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK);

    int conn_rc = connect(sock_fd_, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (conn_rc < 0 && errno != EINPROGRESS) {
        std::cerr << "[STT WS] connect fail: " << strerror(errno) << std::endl;
        ws_close();
        return false;
    }

    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(sock_fd_, &wset);
    struct timeval tv = {5, 0};
    if (select(sock_fd_ + 1, nullptr, &wset, nullptr, &tv) <= 0) {
        std::cerr << "[STT WS] connect timeout" << std::endl;
        ws_close();
        return false;
    }

    fcntl(sock_fd_, F_SETFL, flags);  // 恢复阻塞

    // ── TLS ──
    if (!tls_handshake(host.c_str())) {
        ws_close();
        return false;
    }

    // ── WebSocket 升级 ──
    std::string ws_key = base64_encode(random_bytes(16));

    // 构建 auth URI
    std::string uri = make_auth_url();

    std::ostringstream req;
    req << "GET " << uri << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n"
        << "Upgrade: websocket\r\n"
        << "Connection: Upgrade\r\n"
        << "Sec-WebSocket-Key: " << ws_key << "\r\n"
        << "Sec-WebSocket-Version: 13\r\n"
        << "\r\n";

    std::string req_str = req.str();

    int written = SSL_write(ssl_, req_str.c_str(), req_str.size());
    if (written <= 0) {
        std::cerr << "[STT WS] write upgrade fail" << std::endl;
        ws_close();
        return false;
    }

    // 读响应
    char buf[4096];
    int n = SSL_read(ssl_, buf, sizeof(buf) - 1);
    if (n <= 0) {
        std::cerr << "[STT WS] read upgrade response fail" << std::endl;
        ws_close();
        return false;
    }
    buf[n] = '\0';
    std::string resp(buf, n);

    if (resp.find("101") == std::string::npos &&
        resp.find("Switching Protocols") == std::string::npos) {
        std::cerr << "[STT WS] upgrade rejected:\n" << resp << std::endl;
        ws_close();
        return false;
    }

    std::cerr << "[STT WS] connected OK" << std::endl;
    return true;
}

bool SpeechToText::ws_send_text(const std::string& msg)
{
    if (sock_fd_ < 0 || !ssl_) return false;

    size_t len = msg.size();
    std::string frame;
    frame.reserve(len + 16);

    // FIN + Text opcode
    frame += static_cast<char>(0x81);

    // Masked + payload len
    if (len < 126) {
        frame += static_cast<char>(0x80 | len);
    } else if (len < 65536) {
        frame += static_cast<char>(0x80 | 126);
        frame += static_cast<char>((len >> 8) & 0xFF);
        frame += static_cast<char>(len & 0xFF);
    } else {
        frame += static_cast<char>(0x80 | 127);
        for (int i = 7; i >= 0; --i)
            frame += static_cast<char>((len >> (i * 8)) & 0xFF);
    }

    // Masking key
    std::string mask = random_bytes(4);
    frame += mask;

    // Masked payload
    for (size_t i = 0; i < len; ++i)
        frame += msg[i] ^ mask[i % 4];

    return SSL_write(ssl_, frame.c_str(), frame.size()) > 0;
}

std::string SpeechToText::ws_recv_text(int timeout_ms)
{
    if (sock_fd_ < 0 || !ssl_) return {};

    if (!ws_wait_readable(timeout_ms))
        return {};

    // 读帧头 (至少 2 字节)
    char hdr[2];
    int n = SSL_read(ssl_, hdr, 2);
    if (n != 2) return {};

    unsigned char opcode = hdr[0] & 0x0F;
    bool masked = (hdr[1] & 0x80) != 0;
    uint64_t plen = hdr[1] & 0x7F;

    if (plen == 126) {
        char ext[2];
        if (SSL_read(ssl_, ext, 2) != 2) return {};
        plen = (static_cast<uint64_t>(static_cast<unsigned char>(ext[0])) << 8)
             |  static_cast<uint64_t>(static_cast<unsigned char>(ext[1]));
    } else if (plen == 127) {
        char ext[8];
        if (SSL_read(ssl_, ext, 8) != 8) return {};
        plen = 0;
        for (int i = 0; i < 8; ++i)
            plen = (plen << 8) | static_cast<unsigned char>(ext[i]);
    }

    // Mask key (server → client, should NOT be masked per RFC)
    char mk[4] = {0};
    if (masked) {
        if (SSL_read(ssl_, mk, 4) != 4) return {};
    }

    // Payload
    std::string payload;
    if (plen > 0) {
        // 限制单帧大小
        if (plen > 65536) {
            std::cerr << "[STT WS] frame too large: " << plen << std::endl;
            return {};
        }
        payload.resize(plen);
        size_t total = 0;
        while (total < plen) {
            int r = SSL_read(ssl_, &payload[total], plen - total);
            if (r <= 0) return {};
            total += r;
        }
        if (masked) {
            for (size_t i = 0; i < plen; ++i)
                payload[i] ^= mk[i % 4];
        }
    }

    // Only text frames (opcode 0x1) or close frames (0x8)
    if (opcode == 0x08) {
        std::cerr << "[STT WS] server closed" << std::endl;
        return {};
    }
    if (opcode != 0x01) return {};

    return payload;
}

bool SpeechToText::ws_wait_readable(int timeout_ms)
{
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock_fd_, &rset);
    struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    return select(sock_fd_ + 1, &rset, nullptr, nullptr, &tv) > 0;
}

void SpeechToText::ws_close()
{
    if (ssl_) {
        // 发送 close frame
        char close_frame[] = {
            static_cast<char>(0x88),  // FIN + Close
            static_cast<char>(0x80),  // Masked + len=0
            0, 0, 0, 0               // mask key
        };
        std::string mk = random_bytes(4);
        for (int i = 0; i < 4; ++i) close_frame[2 + i] = mk[i];
        SSL_write(ssl_, close_frame, 6);
    }
    tls_cleanup();
    if (sock_fd_ >= 0) { ::close(sock_fd_); sock_fd_ = -1; }
}

// ═══════════════════════════════════════════
// TLS
// ═══════════════════════════════════════════

bool SpeechToText::tls_handshake(const char* host)
{
    ssl_ctx_ = SSL_CTX_new(TLS_client_method());
    if (!ssl_ctx_) {
        std::cerr << "[STT TLS] SSL_CTX_new fail" << std::endl;
        return false;
    }
    SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_default_verify_paths(ssl_ctx_);

    ssl_ = SSL_new(ssl_ctx_);
    if (!ssl_) {
        std::cerr << "[STT TLS] SSL_new fail" << std::endl;
        return false;
    }

    SSL_set_fd(ssl_, sock_fd_);
    SSL_set_tlsext_host_name(ssl_, host);
    SSL_set_connect_state(ssl_);

    if (SSL_connect(ssl_) != 1) {
        int err = SSL_get_error(ssl_, -1);
        std::cerr << "[STT TLS] connect fail: " << ERR_error_string(err, nullptr) << std::endl;
        return false;
    }

    return true;
}

void SpeechToText::tls_cleanup()
{
    if (ssl_) { SSL_shutdown(ssl_); SSL_free(ssl_); ssl_ = nullptr; }
    if (ssl_ctx_) { SSL_CTX_free(ssl_ctx_); ssl_ctx_ = nullptr; }
}

// ═══════════════════════════════════════════
// 公共接口
// ═══════════════════════════════════════════

SpeechToText::SpeechToText() = default;

SpeechToText::~SpeechToText()
{
    ws_close();
}

void SpeechToText::feed_audio(const int16_t* data, size_t samples)
{
    if (samples == 0 || connect_failed_) return;  // 连接失败则本轮不再重试

    // ── 首帧：建立连接 ──
    if (!first_frame_sent_) {
        if (!ws_connect()) {
            active_ = false;
            connect_failed_ = true;
            std::cerr << "[STT IAT] 鉴权失败，本轮不再重试" << std::endl;
            return;
        }
        active_ = true;
    }

    // ── 音频转 base64 ──
    size_t audio_bytes = samples * sizeof(int16_t);
    std::string raw(reinterpret_cast<const char*>(data), audio_bytes);
    std::string audio_b64 = base64_encode(raw);

    // ── 发送帧 ──
    int status = first_frame_sent_ ? 1 : 0;
    std::string frame = make_frame(status, audio_b64);

    if (!ws_send_text(frame)) {
        std::cerr << "[STT IAT] send frame failed" << std::endl;
        active_ = false;
        return;
    }

    first_frame_sent_ = true;
}

void SpeechToText::end_audio()
{
    if (!active_) return;

    // 发末帧 (status=2, audio 为空)
    std::string frame = make_frame(2, "");
    ws_send_text(frame);
}

// JSON 小工具：提取 "key":"value"
static std::string json_str_val(const std::string& s, const char* key)
{
    std::string k = std::string("\"") + key + "\":\"";
    auto p = s.find(k);
    if (p == std::string::npos) return {};
    p += k.size();
    auto e = s.find('"', p);
    if (e == std::string::npos) return {};
    return s.substr(p, e - p);
}

// JSON 小工具：提取 "key":[int,int]
static std::pair<int,int> json_rg(const std::string& s)
{
    auto p = s.find("\"rg\":[");
    if (p == std::string::npos) return {0,0};
    p += 6; // strlen("\"rg\":[")
    int a = 0, b = 0;
    while (p < s.size() && s[p] >= '0' && s[p] <= '9')
        a = a * 10 + (s[p++] - '0');
    if (p < s.size() && s[p] == ',') ++p;
    while (p < s.size() && s[p] >= '0' && s[p] <= '9')
        b = b * 10 + (s[p++] - '0');
    return {a, b};
}

// 提取 "ws" 数组里所有 "w" 字段拼接
static std::string ws_text(const std::string& s)
{
    std::string out;
    size_t pos = 0;
    while (true) {
        auto wp = s.find("\"w\":\"", pos);
        if (wp == std::string::npos) break;
        wp += 5; // strlen("\"w\":\"")
        auto we = s.find('"', wp);
        if (we == std::string::npos) break;
        out += s.substr(wp, we - wp);
        pos = we + 1;
    }
    return out;
}

std::string SpeechToText::get_result()
{
    if (!active_) return {};

    // IAT 协议：每句 sn→完整句子，rpl 替换，apd 追加
    std::map<int, std::string> sentences;
    bool done = false;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!done && std::chrono::steady_clock::now() < deadline) {
        std::string resp = ws_recv_text(500);
        if (resp.empty()) {
            if (!ws_wait_readable(200)) {
                if (!sentences.empty()) break; // 有结果，提前退出
                continue;
            }
            continue;
        }

        std::cerr << "[STT IAT] recv: " << resp << std::endl;

        auto sn_str = json_str_val(resp, "sn");
        auto pgs    = json_str_val(resp, "pgs");
        auto [rg_a, rg_b] = json_rg(resp);
        std::string text = ws_text(resp);
        bool ls = (resp.find("\"ls\":true") != std::string::npos);

        int sn = sn_str.empty() ? 0 : std::stoi(sn_str);

        if (pgs == "rpl" && rg_a > 0) {
            // 替换 sentences[rg_a .. rg_b]
            sentences[rg_a] = text;
            for (int i = rg_a + 1; i <= rg_b; ++i)
                sentences.erase(i);
        } else if (pgs == "apd") {
            // 追加
            if (sn > 0) {
                auto it = sentences.find(sn);
                if (it != sentences.end())
                    it->second += text;
                else
                    sentences[sn] = text;
            } else {
                sentences[sentences.empty() ? 1 : sentences.rbegin()->first] += text;
            }
        }

        if (ls) done = true;
    }

    // 拼接所有句子
    std::string result;
    for (auto& [sn, txt] : sentences)
        result += txt;

    ws_close();
    active_ = false;
    first_frame_sent_ = false;
    connect_failed_   = false;

    return result;
}

}  // namespace voice
}  // namespace meimei
