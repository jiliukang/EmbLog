#pragma once

#include <array>
#include <atomic>
#include <bit>

#include "Cobs.h"
#include "Config.h"
#include "Encoder.h"
#include "Ports.h"
#include "IndexedRingBuffer.h"
#include "Types.h"

namespace FormatEmbLog
{
template <LoggerAdapter Adapter>
class AsyncManager
{
public:
    AsyncManager();

    template <bool UseCounter, bool UseTimestamp, uint32_t HashID, typename... Args>
    static void PackAndEmit(Level lvl, Args &&... args);

    static void ConsumeAndFlush();

    static void Handle_DMA_TxCpltCallback(uint32_t transmitted_len);

    static void InitializeOSComponents() { Adapter::InitOS(); }

    static uint32_t GetCurrentTimestamp() { return Adapter::GetTimestamp(); }

private:
    inline static std::atomic<uint8_t> seq{0};
    inline static IndexedRingBuffer<> ring_buffer{};
};


template <LoggerAdapter Adapter>
AsyncManager<Adapter>::AsyncManager()
{
    Adapter::InitOS();
}

template <LoggerAdapter Adapter>
template <bool UseCounter, bool UseTimestamp, uint32_t HashID, typename... Args>
void AsyncManager<Adapter>::PackAndEmit(const Level lvl, Args &&... args)
{
    auto norm_args = std::make_tuple(NormalizeArg(std::forward<Args>(args))...);

    uint32_t max_payload = std::apply([](const auto &... ts) { return (0 + ... + EvalSize(ts)); }, norm_args);
    // uint32_t max_payload = (0 + ... + EvalSize(args));

    alignas(4) uint8_t raw[ToSize(PacketSize::CONFIG_BYTE) +
        ToSize(PacketSize::COMP_HEADER) +
        ToSize(PacketSize::COUNTER) +
        ToSize(PacketSize::TIMESTAMP)
        + max_payload + 16];

    uint32_t offset = 0;
    const EmbLogHeader header_obj(lvl, HashID, UseCounter, UseTimestamp);

    auto header_bytes = std::bit_cast<std::array<uint8_t, 4> >(header_obj);

    if constexpr (IsLittleEndian())
    {
        raw[offset++] = header_bytes[0];
        raw[offset++] = header_bytes[1];
        raw[offset++] = header_bytes[2];
        raw[offset++] = header_bytes[3];
    }
    else
    {
        raw[offset++] = header_bytes[3];
        raw[offset++] = header_bytes[2];
        raw[offset++] = header_bytes[1];
        raw[offset++] = header_bytes[0];
    }

    if constexpr (UseCounter)
        raw[offset++] = seq.fetch_add(1, std::memory_order_relaxed);

    if constexpr (UseTimestamp)
    {
        const uint32_t ts = Adapter::GetTimestamp();
        *reinterpret_cast<uint32_t *>(&raw[offset]) = FixEndian(ts);
        offset += 4;
    }
    std::apply([&](auto &&... ts) { (Pack(raw, offset, std::forward<decltype(ts)>(ts)), ...); }, norm_args);

    breakpoint();
    // (Pack(raw, offset, std::forward<Args>(args)), ...);
    if (ring_buffer.PushFrame({raw, offset}))
        Adapter::NotifyData();
}

template <LoggerAdapter Adapter>
void AsyncManager<Adapter>::ConsumeAndFlush()
{
    if (Adapter::IsDmaBusy())
        return;

    const uint32_t total_frames = ring_buffer.frame_count();
    if (total_frames == 0)
        return;

    alignas(4) uint8_t cobs_tx_buffer[4096 + 64];
    uint32_t global_write_idx = 0;
    const std::span<uint8_t> dst_span(cobs_tx_buffer);

    CobsStreamEncoder stream_encoder;

    for (uint32_t i = 0; i < total_frames; ++i)
    {
        LogFrameIndex current_meta;

        if (!ring_buffer.PeekNextFrameMeta(current_meta))
            break;
        const uint32_t single_log_len = current_meta.length;

        if (global_write_idx + cobs_max_encoded_size(single_log_len) > dst_span.size())
            break;

        stream_encoder.Reset(global_write_idx);

        auto body_view = ring_buffer.GetBytesView(current_meta, 0, single_log_len);
        if (body_view.size() == single_log_len)
        {
            stream_encoder.Update(dst_span, body_view);
            global_write_idx = stream_encoder.Finalize(dst_span);
        }
        else
        {
            const auto first_body_len = static_cast<uint32_t>(body_view.size());

            stream_encoder.Update(dst_span, body_view);

            const uint32_t second_body_len = single_log_len - first_body_len;
            const auto body_tail_view = ring_buffer.GetBytesView(current_meta, first_body_len, second_body_len);

            stream_encoder.Update(dst_span, body_tail_view);

            global_write_idx = stream_encoder.Finalize(dst_span);
        }

        ring_buffer.PopFrame(single_log_len);
    }

    if (global_write_idx > 0)
        Adapter::StartDMATx(cobs_tx_buffer, global_write_idx);
}

template <LoggerAdapter Adapter>
void AsyncManager<Adapter>::Handle_DMA_TxCpltCallback(uint32_t transmitted_len)
{
    // 2. 告知适配器 DMA 已经恢复空闲状态
    Adapter::NotifyDmaReady();
}
}