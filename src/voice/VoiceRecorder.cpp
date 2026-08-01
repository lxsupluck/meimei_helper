#include "VoiceRecorder.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace meimei
{
    namespace voice
    {
        void VoiceRecorder::write_wav_header(FILE* fp, uint32_t data_bytes, unsigned int sample_rate)
        {
            uint32_t chunk_size = 36 + data_bytes;
            uint32_t byte_rate = sample_rate * 2; //1ch * 16bit;
            
            uint8_t hdr[44] = {0};
            hdr[0] = 'R'; hdr[1] = 'I'; hdr[2] = 'F'; hdr[3] = 'F'; hdr[4] = chunk_size;
            hdr[5] = chunk_size >> 8; hdr[6] = chunk_size >> 16; hdr[7] = chunk_size >> 24;
            hdr[8] = 'W'; hdr[9] = 'A'; hdr[10] = 'V'; hdr[11] = 'E'; hdr[12] = 'f'; hdr[13] = 'm';
            hdr[14] = 't'; hdr[15] = ' '; hdr[16] = 16; hdr[17] = 0; hdr[18]=0; hdr[19] = 0;
            hdr[20] = 1; hdr[21] = 0; hdr[22] = 1; hdr[23] = 0;
            hdr[24] = sample_rate; hdr[25] = sample_rate >> 8; hdr[26] = sample_rate >> 16;
            hdr[27] = sample_rate >> 24; hdr[28] = byte_rate; hdr[29] = byte_rate >> 8;
            hdr[30] = byte_rate >> 16; hdr[31] = byte_rate >> 24;
            hdr[32] = 2; hdr[33] = 0; hdr[34] = 16; hdr[35] = 0;
            hdr[36] = 'd'; hdr[37] = 'a'; hdr[38] = 't'; hdr[39] = 'a';
            hdr[40] = data_bytes; hdr[41] = data_bytes >> 8; hdr[42] = data_bytes >> 16; 
            hdr[43] = data_bytes >> 24;    

            fwrite(hdr, 1, 44, fp);
        }

        double VoiceRecorder::rms_energy(const int16_t *buf, size_t n)
        {
            if (n == 0)
                return 0.0;
            double sum = 0.0;

            for (size_t i =0; i<n; ++i)
            {
                double v = static_cast<double>(buf[i]);
                sum += v*v;
            }

            return std::sqrt(sum/ static_cast<double>(n));
        }

        bool VoiceRecorder::init(const Config& cfg)
        {
            cfg_ = cfg;

            int rc = snd_pcm_open(&pcm_, cfg_.device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
            if (rc< 0)
            {
                std::cerr << "[VoiceRecorder] 打开设备失败: " << snd_strerror(rc) << std::endl;
                return false;
            }

            rc = snd_pcm_set_params(pcm_, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 
                1,           // 单声道   
                cfg_.sample_rate, 
                1,          //允许重采样 
                500000      //0.5s缓冲
            );

            if (rc < 0){
                std::cerr << "[VoiceRecorder] 设置参数失败: " << snd_strerror(rc) << std::endl;
                snd_pcm_close(pcm_);
                pcm_ = nullptr;
                return false;
            }

            ok_ = true;
            return true;

        }

        void VoiceRecorder::request_stop()
        {
            stop_req_.store(true, std::memory_order_release);
        }

        std::string VoiceRecorder::record()
        {
            if (!ok_) return {};

            stop_req_.store(false, std::memory_order_release);
            const uint16_t rate = cfg_.sample_rate;
            const size_t chunk_sample = rate / 10;
            const uint32_t chunk_max = cfg_.max_record_ms / 100;
            const uint32_t silence_max = cfg_.silence_timeout_ms / 100;

            std::vector<int16_t> chunk(chunk_sample);
            std::vector<int16_t> buffer;
            buffer.reserve(chunk_sample * chunk_max + rate);

            enum{ WAITING, RECORDING} state = WAITING;
            uint32_t silence_count = 0;
            uint32_t total_chunks = 0;

            snd_pcm_prepare(pcm_);

            std::cout << "[VoiceRecorder] 等待说话..." << std::endl;

            while(total_chunks < chunk_max)
            {
                if (stop_req_.load(std::memory_order_acquire)) {
                    std::cout << "[VoiceRecorder] 收到停止请求" << std::endl;
                    break;
                }

                int rc = snd_pcm_readi(pcm_, chunk.data(), chunk_sample);
                if (rc < 0) 
                {
                    snd_pcm_prepare(pcm_);
                    continue;
                }

                size_t n = static_cast<size_t>(rc);
                double r = rms_energy(chunk.data(), n);

                // 流式模式：回调通知
                if (cfg_.on_audio)
                    cfg_.on_audio(chunk.data(), n);

                if(state == WAITING)
                {   
                    if (r > cfg_.speech_threshold)
                    {
                        state = RECORDING;
                        std::cout << "[VoiceRecorder] 检测到语音输入 " << std::endl;
                        if (!cfg_.on_audio || cfg_.debug_save)
                            buffer.insert(buffer.end(), chunk.begin(), chunk.begin()+n);
                        total_chunks = 1;
                        silence_count = 0;
                    }
                     continue;
                }


                if (!cfg_.on_audio || cfg_.debug_save)
                    buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + n);
                total_chunks++;

                if(r < cfg_.speech_threshold)
                {
                    silence_count++;
                    if(silence_count >= silence_max)
                        break;
                }
                else
                {
                    silence_count = 0;
                }
            }

            // 纯流式（不存 WAV）→ 直接返回
            if (cfg_.on_audio && !cfg_.debug_save && buffer.empty())
                return {};

            if (buffer.empty()) return{};

            auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            std::string path = cfg_.output_dir + "voice_" + std::to_string(ts) + ".wav";

            FILE* fp = fopen(path.c_str(), "wb");
            if(!fp) return{};

            uint32_t data_bytes = static_cast<uint32_t>(buffer.size()* sizeof(int16_t));
            write_wav_header(fp,data_bytes, rate);
            fwrite(buffer.data(), 1, data_bytes, fp);
            fclose(fp);

            std::cout << "[VoiceRecorder] 录音文件已保存：" << path << "( " << buffer.size() 
                << " samples)"  << std::endl;
            
            return path;
        }

        VoiceRecorder::~VoiceRecorder()
        {
            if (pcm_) {
                snd_pcm_close(pcm_);
                pcm_ = nullptr;
            }
        }
        
    } // namespace meimei::voice end;

} //namespace meimei end;