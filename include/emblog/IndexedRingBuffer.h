#pragma once
#include <cstdint>
#include <atomic>
#include <cstring>
#include <algorithm>
#include <span>

namespace FormatEmbLog
{
struct LogFrameIndex
{
    uint32_t absolute_start; // 该帧在数据环无限递增总账上的起始位置
    uint32_t length; // 该帧的严格物理总长度
};

template <uint32_t DataBufferSize = 4096, uint32_t IndexQueueSize = 64>
class IndexedRingBuffer
{
    static_assert((DataBufferSize & (DataBufferSize - 1)) == 0, "Data buffer size must be a power of 2!");
    static_assert((IndexQueueSize & (IndexQueueSize - 1)) == 0, "Index queue size must be a power of 2!");

public:
    IndexedRingBuffer() = default;

    [[nodiscard]] __attribute__((always_inline)) inline auto frame_count() const;

    bool PushFrame(std::span<const uint8_t> frame_data);

    bool PeekNextFrameMeta(LogFrameIndex & out_meta);

    std::span<const uint8_t> GetBytesView(const LogFrameIndex & meta, uint32_t relative_offset_in_frame, uint32_t req_len);

    void PopFrame(uint32_t frame_len);

private:
    uint8_t data_storage[DataBufferSize];
    std::atomic<uint32_t> data_head{0};
    std::atomic<uint32_t> data_tail{0};
    static constexpr uint32_t DataMask = DataBufferSize - 1;

    LogFrameIndex index_storage[IndexQueueSize];
    std::atomic<uint32_t> index_head{0};
    std::atomic<uint32_t> index_tail{0};
    static constexpr uint32_t IndexMask = IndexQueueSize - 1;
};

template <uint32_t DataBufferSize, uint32_t IndexQueueSize>
auto IndexedRingBuffer<DataBufferSize, IndexQueueSize>::frame_count() const
{
    return index_head.load(std::memory_order_acquire) - index_tail.load(std::memory_order_relaxed);
}

template <uint32_t DataBufferSize, uint32_t IndexQueueSize>
bool IndexedRingBuffer<DataBufferSize, IndexQueueSize>::PushFrame(const std::span<const uint8_t> frame_data)
{
    if (frame_data.empty())
        return true;
    auto len = static_cast<uint32_t>(frame_data.size());

    const uint32_t ih = index_head.load(std::memory_order_relaxed);
    const uint32_t it = index_tail.load(std::memory_order_acquire);
    if ((ih - it) >= IndexQueueSize)
        return false;

    uint32_t dh = data_head.load(std::memory_order_relaxed);
    const uint32_t dt = data_tail.load(std::memory_order_acquire);
    if ((DataBufferSize - (dh - dt)) < len)
        return false;

    uint32_t offset = dh & DataMask;
    const uint32_t first_chunk = std::min(len, DataBufferSize - offset);
    ::memcpy(&data_storage[offset], frame_data.data(), first_chunk);
    if (len > first_chunk)
    {
        ::memcpy(&data_storage[0], frame_data.data() + first_chunk, len - first_chunk);
    }

    uint32_t idx_offset = ih & IndexMask;
    index_storage[idx_offset] = {.absolute_start = dh, .length = len};
    data_head.store(dh + len, std::memory_order_release);
    index_head.store(ih + 1, std::memory_order_release);
    return true;
}

template <uint32_t DataBufferSize, uint32_t IndexQueueSize>
bool IndexedRingBuffer<DataBufferSize, IndexQueueSize>::PeekNextFrameMeta(LogFrameIndex & out_meta)
{
    const uint32_t ih = index_head.load(std::memory_order_acquire);
    const uint32_t it = index_tail.load(std::memory_order_relaxed);
    if (ih == it)
        return false;

    out_meta = index_storage[it & IndexMask];
    return true;
}

template <uint32_t DataBufferSize, uint32_t IndexQueueSize>
std::span<const uint8_t> IndexedRingBuffer<DataBufferSize, IndexQueueSize>::GetBytesView(
    const LogFrameIndex & meta,
    const uint32_t relative_offset_in_frame,
    const uint32_t req_len)
{
    if (relative_offset_in_frame >= meta.length || req_len == 0)
        return {};

    const uint32_t remaining_in_frame = meta.length - relative_offset_in_frame;
    uint32_t actual_req = std::min(req_len, remaining_in_frame);

    const uint32_t absolute_pos = meta.absolute_start + relative_offset_in_frame;
    uint32_t offset = absolute_pos & DataMask;

    const uint32_t space_to_edge = DataBufferSize - offset;

    if (space_to_edge >= actual_req)
        return std::span<const uint8_t>(&data_storage[offset], actual_req);
    return std::span<const uint8_t>(&data_storage[offset], space_to_edge);
}

template <uint32_t DataBufferSize, uint32_t IndexQueueSize>
void IndexedRingBuffer<DataBufferSize, IndexQueueSize>::PopFrame(const uint32_t frame_len)
{
    const uint32_t dt = data_tail.load(std::memory_order_relaxed);
    const uint32_t it = index_tail.load(std::memory_order_relaxed);

    data_tail.store(dt + frame_len, std::memory_order_release);
    index_tail.store(it + 1, std::memory_order_release); // 索引环向后推进1个槽位
}
}