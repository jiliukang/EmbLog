#pragma once

#include <cstdint>

namespace FormatEmbLog
{
enum class Level : uint8_t
{
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

struct ELFMetadata
{
    uint32_t composite_header;
    const char * fmt_string;
};


enum class PacketSize : uint32_t
{
    CONFIG_BYTE = 1, // Config Byte 占 1 字节
    COMP_HEADER = 3, // 4位Level + 20位Hash ID 复合头占 3 字节
    COUNTER = 1, // 循环序号计数器占 1 字节
    TIMESTAMP = 4 // 32位高精度时间戳占 4 字节
};

inline constexpr uint32_t ToSize(PacketSize sz)
{
    return static_cast<uint32_t>(sz);
}


#pragma pack(push, 1) // 确保结构体在内存中按 1 字节紧凑对齐，无任何空洞
struct EmbLogHeader
{
    // --- Byte 0: Config Byte (从低位到高位排列) ---
    uint32_t reserved: 4; // Bit [3:0]   预留
    uint32_t use_tcobs: 1; // Bit 4       TCOBS 标志
    uint32_t use_counter: 1; // Bit 5       计数器标志
    uint32_t ts_type: 2; // Bit [7:6]   时间戳类型

    // --- Byte 1 ~ 3: 24位复合标识符 ---
    uint32_t hash_id: 20; // Bit [27:8]  20位 FNV-1a 哈希 ID
    uint32_t log_level: 4; // Bit [31:28] 4位 日志安全级别

    // 现代 C++ 风格构造函数：将功能开关直接映射为结构体属性
    inline constexpr EmbLogHeader(const Level lvl, uint32_t id, bool has_cnt, bool has_ts)
        : reserved(0)
        , use_tcobs(false)
        , use_counter(has_cnt ? 1 : 0)
        , ts_type(has_ts ? 2 : 0)
        , // 2 代表 32位 时间戳
        hash_id(id)
        , log_level(static_cast<uint32_t>(lvl))
    {
    }
};

#pragma pack(pop)
static_assert(sizeof(EmbLogHeader) == 4, "FormatEmbLog Error: Header size must be exactly 4 bytes!");

template <typename T = void>
struct GlobalLoggerConfig;
}