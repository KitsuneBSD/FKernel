#pragma once

#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/pair.h>

namespace fkernel {

class VirtualFileSystem;

class PathResolver {
public:
  explicit PathResolver(VirtualFileSystem& vfs, fk::synchronization::Spinlock& lock);

  fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
  resolve(const char* path, fk::RefPtr<Dentry> base = nullptr, int depth = 0);

  fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
  resolve_unlocked(const char* path, fk::RefPtr<Dentry> base = nullptr, int depth = 0);

  fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
  resolve_to_parent(const char* path, int depth = 0);

  fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
  resolve_to_parent_unlocked(const char* path, int depth = 0);

private:
  VirtualFileSystem& m_vfs;
  fk::synchronization::Spinlock& m_lock;
};

} // namespace fkernel
