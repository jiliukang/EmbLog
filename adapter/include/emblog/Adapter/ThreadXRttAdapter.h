#pragma once
#include <cstdint>
#include "threadx-cpp/eventFlags.hpp"
#include <SEGGER_RTT.h>
#include "tx_api.h"
#include "emblog/EmbLog.h"


namespace FormatEmbLog
{
class ThreadXRttAdapter
{
private:
    // SEGGER RTT Channel 0 is the default terminal channel
    static constexpr unsigned int RttChannel = 0;

    // ThreadX synchronization control blocks
    // static inline ThreadX::Native::TX_EVENT_FLAGS_GROUP log_event_flags;
    static inline ThreadX::EventFlags log_event_flags{"EmbLogEventFlags"};
    static inline std::atomic<bool> is_tx_active{false};

    // Event Flag Bitmask (Bit 0 represents "New Data Waiting in Buffer")
    static constexpr ThreadX::Native::ULONG FLG_DATA_READY = 0x00000001U;

public:
    // 1. ThreadX High-precision Timestamp (Converts ThreadX system ticks to microseconds)
    // Alternatively, you can directly read an hardware timer register (e.g., STM32 DWT->CYCCNT)
    static uint32_t GetTimestamp()
    {
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
        // If ThreadX execution profiling is enabled, use high-res time
        return static_cast<uint32_t>(tx_execution_thread_time_get());
#else
        // Fallback to standard system ticks converted to approx ms/us
        return static_cast<uint32_t>(ThreadX::Native::_tx_time_get());
#endif
    }

    // 2. Initialize ThreadX OS primitives
    static void InitOS()
    {
        is_tx_active.store(false, std::memory_order_relaxed);

        // Create a ThreadX Event Flags Group for ultra-fast, interrupt-safe async notifications
        // If called before tx_kernel_enter, ensure it runs inside tx_application_define
        // ThreadX::Native::UINT status = tx_event_flags_create(&log_event_flags, (ThreadX::Native::CHAR *)"EmbLogFlags");
        // if (status != TX_SUCCESS) {
        // Hard fault or loop if kernel primitive allocation fails
        // while(true);
        // }
    }

    // 3. DMA-like Emulation: Pipes memory straight into SEGGER RTT's Up-Buffer
    static void StartDMATx(const void * data, uint32_t len)
    {
        is_tx_active.store(true, std::memory_order_relaxed);

        // SEGGER_RTT_Write is intrinsically thread-safe and safe for concurrent ISR usage.
        // It instantly performs a lock-free memcpy into the RAM-backed RTT Ring Buffer.
        // J-Link HW reads this memory asynchronously via background RAM access (DAP/DMA equivalent).
        unsigned int written = SEGGER_RTT_Write(RttChannel, data, len);

        // Trigger the Tx Complete Callback emulation instantly
        // In a true UART DMA setup, this callback would trigger inside the Uart_DMA_IRQHandler
        // EmbLog_DMA_TxCpltCallback(written);
        // AsyncManager<GlobalLoggerConfig<void>::ActiveAdapter>::Handle_DMA_TxCpltCallback(len);
        StaticLoggerChannel<ThreadXRttAdapter>::instance().Handle_DMA_TxCpltCallback(written);
    }

    // 4. Lock-free Engine Status Checks
    static bool IsDmaBusy()
    {
        return is_tx_active.load(std::memory_order_relaxed);
    }

    static void NotifyDmaReady()
    {
        is_tx_active.store(false, std::memory_order_relaxed);
    }

    // 5. Signal the Background Consumer Task (Safe to execute inside ISRs)
    static void NotifyData()
    {
        // Non-blocking set flag. ThreadX allows tx_event_flags_set from ISRs.
        // tx_event_flags_set(&log_event_flags, FLG_DATA_READY, TX_OR);
        log_event_flags.set(FLG_DATA_READY);
    }

    // 6. Block the Background Consumer Task until data arrives
    static void WaitData()
    {
        auto flags = log_event_flags.waitAny(0xff);

        // ULONG actual_flags;
        // Suspends the background thread efficiently until FLG_DATA_READY is raised.
        // Consumes zero CPU cycles while waiting. Clears the flag automatically upon wakeup.
        // tx_event_flags_get(&log_event_flags, FLG_DATA_READY, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
    }

    // Dummy implementations required to satisfy the LoggerAdapter concept constraints
    static void LockRing()
    {
    }

    static void UnlockRing()
    {
    }

    static void WaitDmaReady()
    {
    }
};
}