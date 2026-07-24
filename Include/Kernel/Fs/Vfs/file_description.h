#pragma once

#include "LibFK/Core/result.h"
#include <Kernel/Fs/Vfs/definitions.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Synchronization/spinlock.h>

class Node;
namespace fkernel { class Dentry; }

class FileDescription : public fk::memory::RefCounted<FileDescription> {
  fk::RefPtr<fkernel::Dentry> m_dentry;
  mutable fk::synchronization::Spinlock m_offset_lock;
  uint64_t m_current_offset{0};
  int m_flags{0};
  bool m_cloexec{false};

public:
  virtual ~FileDescription() override;

  FileDescription(fk::RefPtr<fkernel::Dentry> dentry, int flags);

  fk::core::Result<size_t, fk::core::Error> read(size_t size, uint8_t *buffer);
  fk::core::Result<size_t, fk::core::Error> write(size_t size,
                                                  const uint8_t *buffer);
  fk::core::Result<uint64_t, fk::core::Error> seek(uint64_t offset,
                                                   SeekMode mode);
  fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg);

  uint64_t offset() const { return m_current_offset; }
  void set_offset(uint64_t offset) { m_current_offset = offset; }

  int  open_flags()        const { return m_flags; }
  void set_open_flags(int f)    { m_flags = f; }
  bool is_cloexec()        const { return m_cloexec; }
  void set_cloexec(bool v)      { m_cloexec = v; }

  fk::RefPtr<fkernel::Dentry> dentry() const;
  fk::RefPtr<Node> node() const;
};
