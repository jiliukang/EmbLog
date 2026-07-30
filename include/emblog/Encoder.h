#pragma once

#include <cstdint>
#include <cstring>
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
template <typename... Args>
inline constexpr uint32_t CalculateGenericHash20(const std::string_view str)
{
    uint32_t hash = 0x811C9DC5;
    for (const char c : str)
    {
        hash ^= static_cast<uint8_t>(c);
        hash *= 0x01000193;
    }
    const uint32_t type_salt = (sizeof...(Args) << 16) | (0 + ... + sizeof(Args));
    hash ^= type_salt;

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
        auto sv = std::string_view(arg);
        return sv.substr(0, sv.length() > 254 ? 254 : sv.length());
    }
    else
    {
        return std::forward<T>(arg);
    }
}

template <typename T>
inline void Pack(uint8_t * buf, uint32_t & offset, T && val)
{
    using P = std::decay_t<T>;
    if constexpr (std::is_same_v<P, const char *> || std::is_same_v<P, char *>)
    {
        uint8_t l = 0;
        if (val)
        {
            while (val[l] != '\0' && l < 254)
                l++;
        }
        buf[offset++] = l;
        for (uint8_t i = 0; i < l; ++i)
        {
            buf[offset++] = val[i];
        }
    }
    else if constexpr (std::is_same_v<P, std::string_view> || std::is_same_v<P, std::string>)
    {
        const auto sv = std::string_view(val);
        const auto l = static_cast<uint8_t>(sv.length() > 254 ? 254 : sv.length());
        buf[offset++] = l;
        for (uint8_t i = 0; i < l; ++i)
        {
            buf[offset++] = sv[i];
        }
    }
    else if constexpr (is_vector_v<P>)
    {
        const auto count = static_cast<uint8_t>(val.size() > 255 ? 255 : val.size());
        buf[offset++] = count;

        using VType = P::value_type;
        const uint32_t total_bytes = count * sizeof(VType);

        ::memcpy(&buf[offset], val.data(), total_bytes);

        if constexpr (!IsLittleEndian() && sizeof(VType) > 1)
        {
            auto * p_target = reinterpret_cast<VType *>(&buf[offset]);
            for (uint32_t i = 0; i < count; ++i)
            {
                p_target[i] = FixEndian(p_target[i]);
            }
        }
        offset += total_bytes;
    }
    else if constexpr (std::is_enum_v<P>)
    {
        using EBase = std::underlying_type_t<P>;
        *reinterpret_cast<EBase *>(&buf[offset]) = FixEndian(static_cast<EBase>(val));
        offset += sizeof(EBase);
    }
    else
    {
        *reinterpret_cast<P *>(&buf[offset]) = FixEndian(val);
        offset += sizeof(P);
    }
}
}