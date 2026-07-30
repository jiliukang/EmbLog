#pragma once

#include <cstdint>
#include "Types.h"

#define  ENABLE_EMBLOG 1

/*
  低地址 (先发送) -----------------------------------------------------> 高地址 (后发送)
  +-------------------+-------------------+-------------------+-------------------+

  |  Byte 0 (1 字节)  |  Byte 1 (1 字节)  |  Byte 2 (1 字节)  |  Byte 3 (1 字节)  |
  +-------------------+-------------------+-------------------+-------------------+

  |    Config Byte    |               Composite Identifier (3 字节)               |
  | [可选功能功能控制] | [低20位: FNV-1a 字符串哈希 ID]  +  [高4位: 日志安全级别Level] |
  +-------------------+-------------------+-------------------+-------------------+

*/


namespace FormatEmbLog {
    class ThreadXRttAdapter;


    template<>
    struct GlobalLoggerConfig<void> {
        using ActiveAdapter = ThreadXRttAdapter;
    };


    inline constexpr bool IsLittleEndian() {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return false;
#else
        return true;
#endif
    }

    template<typename T>
    inline constexpr T FixEndian(T val) {
        if constexpr (IsLittleEndian() || sizeof(T) == 1) return val;
        else if constexpr (sizeof(T) == 2) return __builtin_bswap16(static_cast<uint16_t>(val));
        else if constexpr (sizeof(T) == 4) return __builtin_bswap32(static_cast<uint32_t>(val));
        else if constexpr (sizeof(T) == 8) return __builtin_bswap64(static_cast<uint64_t>(val));
        return val;
    }
}
