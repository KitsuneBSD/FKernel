#pragma once

#include "LibFK/Core/Result.h"
#include <Kernel/Fs/Vfs/definitions.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/ref_ptr.h>

class Node;

class FileDescription : public fk::memory::RefCounted<FileDescription> {
  fk::RefPtr<Node> m_node;
  uint64_t m_current_offset{0};
  int m_flags{0};

public:
  virtual ~FileDescription() override;

  FileDescription(fk::RefPtr<Node> node, int flags);

  fk::core::Result<size_t, fk::core::Error> read(size_t size, uint8_t *buffer);
  fk::core::Result<size_t, fk::core::Error> write(size_t size,
                                                  const uint8_t *buffer);
  fk::core::Result<uint64_t, fk::core::Error> seek(uint64_t offset,
                                                   SeekMode mode);
  fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg);

  uint64_t offset() const { return m_current_offset; }
  void set_offset(uint64_t offset) { m_current_offset = offset; }

  fk::RefPtr<Node> node() const;
};
