#include <LibFK/Algorithms/log.h>

namespace fk {
namespace algorithms {

static uint32_t g_log_targets =
    LogTarget::Display |
    LogTarget::DebugFS |
    LogTarget::Serial;

static LogLevel g_log_level = LevelInfo;

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
