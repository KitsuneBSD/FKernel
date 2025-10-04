  #pragma once

  #include <Kernel/Arch/x86_64/io.h>
  #include <LibC/stdint.h>

  static inline uint64_t ticks = 0;

  constexpr uint16_t PIT_CHANNEL0 = 0x40;
  constexpr uint16_t PIT_COMMAND = 0x43;
  constexpr uint8_t PIT_CMD_RATE_GEN = 0x34;

  class PIT {
  private:
    PIT() = default;

    void set_frequency(uint32_t frequency);

  public:
    static PIT &the() {
      static PIT inst;
      return inst;
    }

    void initialize(uint32_t frequency);
    void sleep(uint64_t ms);
    uint64_t get_ticks() const { return ticks; }
  };
