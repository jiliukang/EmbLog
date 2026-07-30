#pragma once

#include <cstdint>
#include <span>

namespace FormatEmbLog {

    class CobsStreamEncoder {
    private:
        uint32_t code_idx = 0;   // 当前帧在目标缓冲区中的“距离指针”绝对物理位置
        uint8_t code_val = 1;    // 当前距离上一个 0x00 的计数器
        uint32_t write_idx = 0;  // 目标缓冲区的写入位置

    public:
        // 🟢 初始化状态：为新的一条日志开启 COBS 流式编码
        // dst_offset: 当前整个大物理发送缓冲区的当前写入偏置
        inline void Reset(uint32_t dst_offset) {
            code_idx = dst_offset;
            code_val = 1;
            write_idx = dst_offset + 1; // 为第一个计数器留空
        }

        // 🟢 多次调用核心：对任意一段连续的 Raw 原始数据进行 COBS 转码追加
        // dst: 整个大物理发送缓冲区视图
        // src: 这一段连续的原始日志数据片段视图
        inline void Update(std::span<uint8_t> dst, std::span<const uint8_t> src) {
            if (src.empty()) return;
            uint32_t read_idx = 0;
            uint32_t src_len = static_cast<uint32_t>(src.size());

            while (read_idx < src_len) {
                uint8_t current_byte = src[read_idx++];

                if (current_byte == 0x00) {
                    dst[code_idx] = code_val;
                    code_idx = write_idx++;
                    code_val = 1;
                } else {
                    dst[write_idx++] = current_byte;
                    code_val++;

                    if (code_val == 0xFF) {
                        dst[code_idx] = code_val;
                        code_idx = write_idx++;
                        code_val = 1;
                    }
                }
            }
        }

        // 🟢 帧完成函数：一条日志全部 Update 完毕后调用，回填最后计数器并添加末尾 0
        // 返回值：当前这条日志经过 COBS 编码后，写出的【绝对物理总长度】
        inline uint32_t Finalize(std::span<uint8_t> dst) {
            dst[code_idx] = code_val;   // 回填最后的计数器
            dst[write_idx++] = 0x00;    // 🟢 注入物理分帧终结符 0x00

            return write_idx; // 返回此时大发送缓冲区的新偏移位置
        }
    };
    /**
     * @brief 辅助函数：根据输入长度计算 COBS 编码所需的最大缓冲区大小
     */
    constexpr std::size_t cobs_max_encoded_size(std::size_t source_size) noexcept
    {
        // 每 254 个字节最多增加 1 个字节，外加 1 个起始 code 和 1 个末尾 0x00
        return source_size + (source_size / 254) + 2;
    }
};