#include <LibFK/Algorithms/Logging/log.h>

namespace fk {
namespace algorithms {

static uint32_t g_log_targets =
    LogTarget::Display |
    LogTarget::DebugFS |
    LogTarget::Serial;

#if FKERNEL_LOG_LEVEL >= 4
static LogLevel g_log_level = LevelTrace;
#elif FKERNEL_LOG_LEVEL >= 3
static LogLevel g_log_level = LevelDebug;
#elif FKERNEL_LOG_LEVEL >= 2
static LogLevel g_log_level = LevelInfo;
#elif FKERNEL_LOG_LEVEL >= 1
static LogLevel g_log_level = LevelWarn;
#else
static LogLevel g_log_level = LevelError;
#endif

void set_log_targets(uint32_t targets) {
    g_log_targets = targets;
}

uint32_t get_log_targets() {
    return g_log_targets;
}

void set_log_level(LogLevel level) {
    g_log_level = level;
}

LogLevel get_log_level() {
    return g_log_level;
}

} // namespace algorithms
} // namespace fk
