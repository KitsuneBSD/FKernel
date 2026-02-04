#include <Kernel/Driver/Storage/Nvme/NvmeRegisterAccess.h>
#include <LibC/string.h>

namespace fkernel {

NvmeRegisterAccess::NvmeRegisterAccess(uintptr_t controller_phys_addr)
    : m_registers(reinterpret_cast<volatile uint8_t*>(controller_phys_addr)) {
  // Read capabilities to determine page size
  uint64_t cap = read_capabilities();
  m_page_size = 4096 << ((cap >> 0) & 0xF); // MPS (Memory Page Size)
}

uint64_t NvmeRegisterAccess::read_capabilities() const {
  return *reinterpret_cast<volatile uint64_t*>(m_registers + 0x00);
}

uint32_t NvmeRegisterAccess::read_version() const {
  return *reinterpret_cast<volatile uint32_t*>(m_registers + 0x08);
}

uint32_t NvmeRegisterAccess::read_cc() const {
  return *reinterpret_cast<volatile uint32_t*>(m_registers + 0x14);
}

void NvmeRegisterAccess::write_cc(uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x14) = value;
}

uint32_t NvmeRegisterAccess::read_csts() const {
  return *reinterpret_cast<volatile uint32_t*>(m_registers + 0x1C);
}

uint32_t NvmeRegisterAccess::read_aqa() const {
  return *reinterpret_cast<volatile uint32_t*>(m_registers + 0x24);
}

void NvmeRegisterAccess::write_aqa(uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x24) = value;
}

uint64_t NvmeRegisterAccess::read_asq() const {
  return *reinterpret_cast<volatile uint64_t*>(m_registers + 0x28);
}

void NvmeRegisterAccess::write_asq(uint64_t value) {
  *reinterpret_cast<volatile uint64_t*>(m_registers + 0x28) = value;
}

uint64_t NvmeRegisterAccess::read_acq() const {
  return *reinterpret_cast<volatile uint64_t*>(m_registers + 0x30);
}

void NvmeRegisterAccess::write_acq(uint64_t value) {
  *reinterpret_cast<volatile uint64_t*>(m_registers + 0x30) = value;
}

uint32_t NvmeRegisterAccess::read_intms() const {
  return *reinterpret_cast<volatile uint32_t*>(m_registers + 0x0C);
}

void NvmeRegisterAccess::write_intms(uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x0C) = value;
}

uint32_t NvmeRegisterAccess::read_intmc() const {
  return *reinterpret_cast<volatile uint32_t*>(m_registers + 0x10);
}

void NvmeRegisterAccess::write_intmc(uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x10) = value;
}

void NvmeRegisterAccess::write_cmd_register(uint8_t command, uint32_t nsid, uint64_t prp1,
                                            uint64_t prp2) {
  uint32_t cdw0 = command;
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x1000) = cdw0;
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x1004) = nsid;
  *reinterpret_cast<volatile uint64_t*>(m_registers + 0x1008) = prp1;
  *reinterpret_cast<volatile uint64_t*>(m_registers + 0x1010) = prp2;
}

} // namespace fkernel