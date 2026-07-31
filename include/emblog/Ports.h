#pragma once

#include <cstdint>
#include <concepts>

namespace FormatEmbLog
{
template <typename T>
concept LoggerAdapter = requires
{
    { T::GetTimestamp() } -> std::same_as<uint32_t>;
    { T::StartDMATx(static_cast<const void *>(nullptr), static_cast<uint32_t>(0)) } -> std::same_as<void>;
    { T::InitOS() } -> std::same_as<void>;
    { T::LockRing() } -> std::same_as<void>;
    { T::UnlockRing() } -> std::same_as<void>;
    { T::NotifyData() } -> std::same_as<void>;
    { T::WaitData() } -> std::same_as<void>;
    { T::WaitDmaReady() } -> std::same_as<void>;
    { T::NotifyDmaReady() } -> std::same_as<void>;
};
}