#pragma once

#include <Kernel/Hardware/Pci/pci.h>
#include <LibFK/Core/Result.h>

namespace fkernel {

class NvmeRegisterAccess {
public:
  NvmeRegisterAccess(uintptr_t controller_phys_addr);

  // Register access methods
  uint64_t read_capabilities() const;
  uint32_t read_version() const;
  uint32_t read_cc() const;
  void write_cc(uint32_t value);
  uint32_t read_csts() const;
  uint32_t read_aqa() const;
  void write_aqa(uint32_t value);
  uint64_t read_asq() const;
  void write_asq(uint64_t value);
  uint64_t read_acq() const;
  void write_acq(uint64_t value);
  uint32_t read_intms() const;
  void write_intms(uint32_t value);
  uint32_t read_intmc() const;
  void write_intmc(uint32_t value);

  // NVMe command registers
  void write_cmd_register(uint8_t command, uint32_t nsid, uint64_t prp1, uint64_t prp2);

  // Helper methods
  uint32_t get_page_size() const { return m_page_size; }

private:
  volatile uint8_t* m_registers;
  uint32_t m_page_size;
};

} // namespace fkernel