#pragma once

#include <Kernel/Fs/Vfs/dentry_node_stack.h>
#include <LibFK/Container/hash_map.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

class Dentry;

class MountNamespace : public fk::memory::RefCounted<MountNamespace> {
public:
  struct MountRecord {
    char path[128];
    char fstype[16];
    uint32_t dev_id;
    Dentry* dentry_ptr;
  };

  static fk::RefPtr<MountNamespace> create(fk::RefPtr<Dentry> root_dentry);
  ~MountNamespace();

  fk::RefPtr<Dentry> root_dentry() const { return m_root_dentry; }

  DentryNodeStack* get_stack(Dentry* d);
  DentryNodeStack& ensure_stack(Dentry* d);

  void push_mount(Dentry* d, fk::RefPtr<Node> node, const char* path, const char* fstype);
  void pop_mount(const char* path);
  void update_root_mount(const char* new_fstype);
  void add_mount_entry(const char* path, const char* fstype, Dentry* d);

  const MountRecord* mounts() const { return m_mounts; }
  size_t mount_count() const { return m_mount_count; }
  uint32_t next_dev_id() { return m_next_dev_id++; }

  uint32_t dev_id_for_path(const char* path) const;

  MountNamespace() = default;

private:
  mutable fk::synchronization::Spinlock m_lock;
  fk::RefPtr<Dentry> m_root_dentry;
  fk::containers::HashMap<Dentry*, DentryNodeStack*> m_stacks;

  MountRecord m_mounts[64];
  size_t m_mount_count{0};
  uint32_t m_next_dev_id{1};
};

MountNamespace* current_mount_namespace();

} // namespace fkernel

namespace fk {
namespace containers {
template <>
struct DefaultHasher<fkernel::Dentry*> {
  size_t operator()(fkernel::Dentry* const& value) const {
    return reinterpret_cast<size_t>(value);
  }
};
} // namespace containers
} // namespace fk
