#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
VirtualFileSystem::resolve_path(const char* path, fk::RefPtr<Dentry> base, int depth) {
  return m_resolver.resolve(path, base, depth);
}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
VirtualFileSystem::resolve_path_unlocked(const char* path, fk::RefPtr<Dentry> base, int depth) {
  return m_resolver.resolve_unlocked(path, base, depth);
}

fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
VirtualFileSystem::resolve_path_to_parent(const char* path, int depth) {
  return m_resolver.resolve_to_parent(path, depth);
}

fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
VirtualFileSystem::resolve_path_to_parent_unlocked(const char* path, int depth) {
  return m_resolver.resolve_to_parent_unlocked(path, depth);
}

} // namespace fkernel
