#pragma once

#include <cstdint>
#include <cstring>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Base/breakpoint.h"

#include "Config.h"

template <typename T>
struct is_vector : std::false_type
{
};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc> > : std::true_type
{
};

template <typename T>
inline constexpr bool is_vector_v = is_vector<std::decay_t<T> >::value;


namespace FormatEmbLog
{
inline constexpr uint32_t CalculateGenericHash20(const std::string_view str,const std::source_location location = std::source_location::current())
{
    uint32_t hash = 0x811C9DC5;
    for (const char c : str)
    {
        hash ^= static_cast<uint8_t>(c);
        hash *= 0x01000193;
    }
    const std::string_view file_name {location.file_name()};
    const auto line_num =  location.line();
    for (const char c : file_name) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 0x01000193;
    }

    hash ^= static_cast<uint8_t>(line_num & 0xFF); hash *= 0x01000193;
    hash ^= static_cast<uint8_t>((line_num >> 8) & 0xFF); hash *= 0x01000193;
    hash ^= static_cast<uint8_t>((line_num >> 16) & 0xFF); hash *= 0x01000193;
    hash ^= static_cast<uint8_t>((line_num >> 24) & 0xFF); hash *= 0x01000193;

    return hash & 0xFFFFF;
}


template <typename T>
inline uint32_t EvalSize(const T & val)
{
    using P = std::decay_t<T>;
    if constexpr (is_vector_v<P>)
        return 1 + (static_cast<uint32_t>(val.size()) * sizeof(typename P::value_type));
    else if constexpr (std::is_same_v<P, const char *> || std::is_same_v<P, char *>)
    {
        uint32_t l = 0;
        if (val)
        {
            while (val[l] != '\0' && l < 254)
                l++;
        }
        return 1 + l;
    }
    else if constexpr (std::is_same_v<P, std::string_view> || std::is_same_v<P, std::string>)
    {
        auto sv = std::string_view(val);
        return 1 + static_cast<uint32_t>(sv.length() > 254 ? 254 : sv.length());
    }
    else if constexpr (std::is_enum_v<P>)
        return sizeof(std::underlying_type_t<P>);
    else
        return sizeof(P);
}

template <typename T>
static inline auto NormalizeArg(T && arg)
{
    using P = std::decay_t<T>;
    if constexpr (std::is_same_v<P, const char *> || std::is_same_v<P, char *>)
    {
        if (!arg)
            return std::string_view("");
        uint32_t l = 0;
        while (arg[l] != '\0' && l < 254)
            l++;
        return std::string_view(arg, l);
    }
    else if constexpr (std::is_same_v<P, std::string>)
    {
        const auto sv = std::string_view(arg);
        return sv.substr(0, sv.length() > 254 ? 254 : sv.length());
    }
    else
    {
        return std::forward<T>(arg);
    }
}

template <typename T>
inline void Pack(uint8_t * buf, uint32_t & offset, T && val) {
    using P = std::decay_t<T>;
    if constexpr (std::is_same_v<P, std::string_view>) {
        const auto l = static_cast<uint8_t>(val.length());
        buf[offset++] = l;
        if (l > 0) {
            ::memcpy(&buf[offset], val.data(), l);
            offset += l;
        }
    } else if constexpr (is_vector_v<P>) {
        const auto count = static_cast<uint8_t>(val.size() > 255 ? 255 : val.size());
        buf[offset++] = count;
        using VType = typename P::value_type;
        const uint32_t total_bytes = count * sizeof(VType);

        ::memcpy(&buf[offset], val.data(), total_bytes);
        if constexpr (!IsLittleEndian() && sizeof(VType) > 1) {
            auto * p_target = reinterpret_cast<VType *>(&buf[offset]);
            for (uint32_t i = 0; i < count; ++i) {
                p_target[i] = FixEndian(p_target[i]);
            }
        }
        offset += total_bytes;
    } else if constexpr (std::is_enum_v<P>) {
        using EBase = std::underlying_type_t<P>;
        EBase fixed_val = FixEndian(static_cast<EBase>(val));
        ::memcpy(&buf[offset], &fixed_val, sizeof(EBase));
        offset += sizeof(EBase);
    } else {
        P fixed_val = FixEndian(val);
        ::memcpy(&buf[offset], &fixed_val, sizeof(P));
        offset += sizeof(P);
    }
}

}