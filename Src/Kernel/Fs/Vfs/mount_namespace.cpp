#include <Kernel/Fs/Vfs/mount_namespace.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fkernel {

fk::RefPtr<MountNamespace> MountNamespace::create(fk::RefPtr<Dentry> root_dentry) {
  auto ns = fk::make_ref<MountNamespace>().value();
  ns->m_root_dentry = root_dentry;
  return ns;
}

MountNamespace::~MountNamespace() {
  for (size_t i = 0; i < m_mount_count; ++i) {
    auto* stack = get_stack(m_mounts[i].dentry_ptr);
    if (stack) {
      m_stacks.remove(m_mounts[i].dentry_ptr);
      delete stack;
    }
  }
}

DentryNodeStack* MountNamespace::get_stack(Dentry* d) {
  auto opt = m_stacks.get(d);
  if (opt.has_value())
    return opt.value();
  return nullptr;
}

DentryNodeStack& MountNamespace::ensure_stack(Dentry* d) {
  auto* existing = get_stack(d);
  if (existing) return *existing;
  auto* new_stack = new DentryNodeStack();
  m_stacks.insert(d, new_stack);
  return *new_stack;
}

void MountNamespace::push_mount(Dentry* d, fk::RefPtr<Node> node, const char* path, const char* fstype) {
  fk::synchronization::ScopedLock lock(m_lock);
  auto& stack = ensure_stack(d);
  stack.push(node);
  add_mount_entry(path, fstype, d);
}

void MountNamespace::pop_mount(const char* path) {
  fk::synchronization::ScopedLock lock(m_lock);
  for (size_t i = 0; i < m_mount_count; ++i) {
    if (__builtin_strcmp(m_mounts[i].path, path) == 0) {
      m_mounts[i] = m_mounts[--m_mount_count];
      break;
    }
  }
}

void MountNamespace::update_root_mount(const char* new_fstype) {
  fk::synchronization::ScopedLock lock(m_lock);
  for (size_t i = 0; i < m_mount_count; ++i) {
    if (__builtin_strcmp(m_mounts[i].path, "/") == 0) {
      fk::memory::copy_n(m_mounts[i].fstype, new_fstype, 15);
      m_mounts[i].fstype[15] = '\0';
      return;
    }
  }
}

void MountNamespace::add_mount_entry(const char* path, const char* fstype, Dentry* d) {
  if (m_mount_count >= 64) return;
  fk::memory::copy_n(m_mounts[m_mount_count].path, path, 127);
  m_mounts[m_mount_count].path[127] = '\0';
  fk::memory::copy_n(m_mounts[m_mount_count].fstype, fstype ? fstype : "auto", 15);
  m_mounts[m_mount_count].fstype[15] = '\0';
  m_mounts[m_mount_count].dev_id = m_next_dev_id++;
  m_mounts[m_mount_count].dentry_ptr = d;
  ++m_mount_count;
}

uint32_t MountNamespace::dev_id_for_path(const char* path) const {
  if (!path) return 1;
  size_t best_len = 0;
  uint32_t best_dev = 1;
  fk::synchronization::ScopedLock lock(m_lock);
  for (size_t i = 0; i < m_mount_count; ++i) {
    size_t mp_len = __builtin_strlen(m_mounts[i].path);
    if (__builtin_strncmp(path, m_mounts[i].path, mp_len) == 0 && mp_len > best_len) {
      best_len = mp_len;
      best_dev = m_mounts[i].dev_id;
    }
  }
  return best_dev;
}

MountNamespace* current_mount_namespace() {
  auto* task = SchedulerManager::the().current();
  if (task && task->resources.files.mount_ns)
    return task->resources.files.mount_ns.get();
  return nullptr;
}

} // namespace fkernel
