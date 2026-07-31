#pragma once

#include <emio/emio.hpp>

#include "AsyncEngine.h"
#include "Config.h"
#include "Ports.h"
#include "Types.h"

#if defined(ENABLE_EMBLOG) && (ENABLE_EMBLOG == 1)

namespace FormatEmbLog
{
template <LoggerAdapter Adapter>
struct StaticLoggerChannel
{
    static inline AsyncManager<Adapter> & instance()
    {
        static AsyncManager<Adapter> instance_{};
        return instance_;
    }
};

template <bool UseCounter, bool UseTimestamp, uint32_t HashID, typename T = void, typename... Args>
inline void CheckedLogEmit(Level lvl, ::emio::format_string<Args...> fmt_str, Args &&... args)
{
    using ChosenAdapter = typename GlobalLoggerConfig<T>::ActiveAdapter;
    StaticLoggerChannel<ChosenAdapter>::instance().template PackAndEmit<UseCounter, UseTimestamp, HashID>(
        lvl,
        std::forward<Args>(args)...);
}
}

#    define emb_log(level,counter,timestamp,fmt, ...) \
        do { \
        constexpr uint32_t hash_id = FormatEmbLog::CalculateGenericHash20(fmt)| (static_cast<uint32_t>(level) << 20); \
            FormatEmbLog::CheckedLogEmit<counter, timestamp, hash_id>(level, fmt __VA_OPT__(, )##__VA_ARGS__); \
        } while (0)

#    define log_debug(fmt, ...) emb_log(FormatEmbLog::Level::DEBUG,false,false,fmt,##__VA_ARGS__)
#    define log_debug_ts(fmt, ...) emb_log(FormatEmbLog::Level::INFO,false,true,fmt,##__VA_ARGS__)
#    define log_debug_cnt(fmt, ...) emb_log(FormatEmbLog::Level::INFO,true,false,fmt,##__VA_ARGS__)
#    define log_debug_full(fmt, ...)  emb_log(FormatEmbLog::Level::INFO,true,true,fmt,##__VA_ARGS__)

#    define log_info(fmt, ...) emb_log(FormatEmbLog::Level::INFO,false,false,fmt,##__VA_ARGS__)
#    define log_info_ts(fmt, ...) emb_log(FormatEmbLog::Level::INFO,false,true,fmt,##__VA_ARGS__)
#    define log_info_cnt(fmt, ...) emb_log(FormatEmbLog::Level::INFO,true,false,fmt,##__VA_ARGS__)
#    define log_info_full(fmt, ...)  emb_log(FormatEmbLog::Level::INFO,true,true,fmt,##__VA_ARGS__)


#    define log_warn(fmt, ...) emb_log(FormatEmbLog::Level::WARN,false,false,fmt,##__VA_ARGS__)
#    define log_warn_ts(fmt, ...) emb_log(FormatEmbLog::Level::WARN,false,true,fmt,##__VA_ARGS__)
#    define log_warn_cnt(fmt, ...) emb_log(FormatEmbLog::Level::WARN,true,false,fmt,##__VA_ARGS__)
#    define log_warn_full(fmt, ...)  emb_log(FormatEmbLog::Level::WARN,true,true,fmt,##__VA_ARGS__)

#    define log_error(fmt, ...) emb_log(FormatEmbLog::Level::ERROR,false,false,fmt,##__VA_ARGS__)
#    define log_error_ts(fmt, ...) emb_log(FormatEmbLog::Level::ERROR,false,true,fmt,##__VA_ARGS__)
#    define log_error_cnt(fmt, ...) emb_log(FormatEmbLog::Level::ERROR,true,false,fmt,##__VA_ARGS__)
#    define log_error_full(fmt, ...)  emb_log(FormatEmbLog::Level::ERROR,true,true,fmt,##__VA_ARGS__)

#    define log_fatal(fmt, ...) emb_log(FormatEmbLog::Level::FATAL,false,false,fmt,##__VA_ARGS__)
#    define log_fatal_ts(fmt, ...) emb_log(FormatEmbLog::Level::FATAL,false,true,fmt,##__VA_ARGS__)
#    define log_fatal_cnt(fmt, ...) emb_log(FormatEmbLog::Level::FATAL,true,false,fmt,##__VA_ARGS__)
#    define log_fatal_full(fmt, ...)  emb_log(FormatEmbLog::Level::FATAL,true,true,fmt,##__VA_ARGS__)

#else
#    define log_debug(fmt, ...)do  { } while (0)
#    define log_debug_ts(fmt, ...)do  { } while (0)
#    define log_debug_cnt(fmt, ...)do  { } while (0)
#    define log_debug_full(fmt, ...)do  { } while (0)

#    define log_info(fmt, ...)do  { } while (0)
#    define log_info_ts(fmt, ...)do  { } while (0)
#    define log_info_cnt(fmt, ...)do  { } while (0)
#    define log_info_full(fmt, ...)do  { } while (0)


#    define log_warn(fmt, ...)do  { } while (0)
#    define log_warn_ts(fmt, ...)do  { } while (0)
#    define log_warn_cnt(fmt, ...)do  { } while (0)
#    define log_warn_full(fmt, ...)do  { } while (0)

#    define log_error(fmt, ...)do  { } while (0)
#    define log_error_ts(fmt, ...)do  { } while (0)
#    define log_error_cnt(fmt, ...)do  { } while (0)
#    define log_error_full(fmt, ...)do  { } while (0)

#    define log_fatal(fmt, ...)do  { } while (0)
#    define log_fatal_ts(fmt, ...)do  { } while (0)
#    define log_fatal_cnt(fmt, ...)do  { } while (0)
#    define log_fatal_full(fmt, ...)do  { } while (0)

#endif