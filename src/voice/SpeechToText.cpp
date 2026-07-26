#include "SpeechToText.h"
#include "Config.h"

#include <curl/curl.h>
#include <openssl/md5.h>

#include <fstream>
#include <cstring>
#include <ctime>
#include <iostream>

namespace meimei {
namespace voice {

// ── Base64 编码 ────────────────────────────

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

// ── MD5 返回 32 位小写 hex ─────────────────

static std::string md5_hex(const std::string& in)
{
    unsigned char d[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(in.c_str()), in.size(), d);
    char hex[33];
    for (int i = 0; i < 16; ++i)
        std::sprintf(hex + i * 2, "%02x", d[i]);
    return std::string(hex);
}

// ── libcurl 写回调 ──────────────────────────

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

// ── 主函数 ──────────────────────────────────

std::string SpeechToText::transcribe(const std::string& wav_path)
{
    using config::STT_API_URL;
    using config::STT_APP_ID;
    using config::STT_API_KEY;

    // 1. 读 WAV
    std::ifstream file(wav_path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "[STT] 无法打开文件: " << wav_path << std::endl;
        return {};
    }
    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string audio(sz, '\0');
    file.read(audio.data(), sz);
    file.close();

    // 2. 讯飞签名
    constexpr const char* PARAM_JSON =
        R"({"language":"zh_cn","accent":"mandarin"})";
    std::string param_b64 = base64_encode(PARAM_JSON);
    std::string cur_time  = std::to_string(std::time(nullptr));
    std::string checksum  = md5_hex(STT_API_KEY + cur_time + param_b64);

    // 3. curl
    auto* curl = curl_easy_init();
    if (!curl) return {};

    std::string response;
    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, ("X-Appid: "    + std::string(STT_APP_ID)).c_str());
    hdrs = curl_slist_append(hdrs, ("X-CurTime: "  + cur_time).c_str());
    hdrs = curl_slist_append(hdrs, ("X-Param: "    + param_b64).c_str());
    hdrs = curl_slist_append(hdrs, ("X-CheckSum: " + checksum).c_str());
    hdrs = curl_slist_append(hdrs, "Content-Type: application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_URL, STT_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, audio.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(audio.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[STT] curl 失败: " << curl_easy_strerror(res) << std::endl;
        return {};
    }

    // 4. 脱敏 — 抠 "sn" 内的 "w"
    //    讯飞返回格式: {"code":"0","data":{"result":{"ws":[{"cw":[{"w":"你好"}]}]}}}
    std::cout << "[STT] 原始返回: " << response << std::endl;

    std::string text;
    size_t pos = 0;
    while (true) {
        pos = response.find("\"w\":\"", pos);
        if (pos == std::string::npos) break;
        pos += 5;  // 跳过 "w":"
        size_t end = response.find('"', pos);
        if (end == std::string::npos) break;
        text += response.substr(pos, end - pos);
        pos = end + 1;
    }

    return text;
}

}  // namespace voice
}  // namespace meimei
