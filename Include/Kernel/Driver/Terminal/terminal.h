#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Text/string.h>

namespace fkernel {
namespace terminal {

class InputDevice;
class OutputDevice;

struct TerminalCapabilities {
    bool supports_color{false};
    bool supports_raw_mode{false};
    bool supports_canonical_mode{false};
    uint16_t max_rows{25};
    uint16_t max_cols{80};
};

/// @brief Abstract base class for all terminal types
class Terminal : public CharacterDevice {
public:
    virtual ~Terminal() = default;
    
    /// @brief Attach an input device to this terminal
    virtual fk::core::Result<void, fk::core::Error> attach_input(InputDevice* device) = 0;
    
    /// @brief Attach an output device to this terminal  
    virtual fk::core::Result<void, fk::core::Error> attach_output(OutputDevice* device) = 0;
    
    /// @brief Get terminal capabilities
    virtual TerminalCapabilities capabilities() const = 0;
    
    /// @brief Set terminal size
    virtual fk::core::Result<void, fk::core::Error> set_size(uint16_t rows, uint16_t cols) = 0;
    
    /// @brief Get terminal size
    virtual void get_size(uint16_t& rows, uint16_t& cols) const = 0;
    
    /// @brief Terminal type identification
    virtual const char* type_name() const = 0;
    
protected:
    TerminalCapabilities m_capabilities;
};

} // namespace terminal
} // namespace fkernel