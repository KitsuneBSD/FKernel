#pragma once

#include <LibC/stdint.h>

namespace fkernel {

class NvmeDeviceConfiguration {
public:
  NvmeDeviceConfiguration() = default;

  void set_page_size(uint32_t size) { m_page_size = size; }
  uint32_t page_size() const { return m_page_size; }

  void set_block_size(uint32_t size) { m_block_size = size; }
  uint32_t block_size() const { return m_block_size; }

  void set_namespace_id(uint32_t id) { m_namespace_id = id; }
  uint32_t namespace_id() const { return m_namespace_id; }

  void set_controller_page_size(uint32_t size) { m_controller_page_size = size; }
  uint32_t controller_page_size() const { return m_controller_page_size; }

  void set_total_blocks(uint64_t blocks) { m_total_blocks = blocks; }
  uint64_t total_blocks() const { return m_total_blocks; }

  bool is_valid() const { return m_page_size > 0 && m_block_size > 0 && m_namespace_id > 0; }

private:
  uint32_t m_page_size = 512;
  uint32_t m_block_size = 512;
  uint32_t m_namespace_id = 1;
  uint32_t m_controller_page_size = 4096;
  uint64_t m_total_blocks = 0;
};

} // namespace fkernel
