#include <Kernel/Fs/Virtual/ProcFs/proc_sys_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_sys_kernel_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_sys_string_node.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Algorithms/string_algorithms.h>
#include <LibFK/Utilities/memory.h>

char g_proc_hostname[64]   = "fkernel";
char g_proc_domainname[64] = "(none)";

// --- ProcSysNode (/proc/sys/) ---

fk::core::Result<void, fk::core::Error>
ProcSysNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  DirectoryEntry de;
  fk::memory::copy_n(de.name, "kernel", sizeof(de.name));
  de.type = 1;
  entries.push_back(de);
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
ProcSysNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  if (fk::memory::compare(name, "kernel") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSysKernelNode>().value());
  return fk::core::Error::NotFound;
}

// --- ProcSysKernelNode (/proc/sys/kernel/) ---

static constexpr const char OSTYPE[]    = "Linux\n";
static constexpr const char OSRELEASE[] = "5.15.0-fkernel\n";

fk::core::Result<void, fk::core::Error>
ProcSysKernelNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  const char* names[] = { "hostname", "ostype", "osrelease", "domainname" };
  for (auto* n : names) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, n, sizeof(de.name));
    de.type = 0;
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
ProcSysKernelNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  if (fk::memory::compare(name, "hostname") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSysStringNode>(g_proc_hostname, g_proc_hostname).value());
  if (fk::memory::compare(name, "ostype") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSysStringNode>(OSTYPE).value());
  if (fk::memory::compare(name, "osrelease") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSysStringNode>(OSRELEASE).value());
  if (fk::memory::compare(name, "domainname") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSysStringNode>(g_proc_domainname, g_proc_domainname).value());
  return fk::core::Error::NotFound;
}

// --- ProcSysStringNode ---

fk::core::Result<size_t, fk::core::Error>
ProcSysStringNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  size_t len = 0;
  while (m_value[len]) ++len;
  // append '\n' if not already present
  bool needs_newline = (len == 0 || m_value[len - 1] != '\n');
  size_t total = len + (needs_newline ? 1 : 0);
  if ((size_t)offset >= total) return static_cast<size_t>(0);
  size_t available = total - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) {
    size_t pos = (size_t)offset + i;
    buffer[i] = (pos < len) ? static_cast<uint8_t>(m_value[pos]) : '\n';
  }
  return to_copy;
}

fk::core::Result<size_t, fk::core::Error>
ProcSysStringNode::write(uint64_t, size_t size, const uint8_t* buffer) {
  if (!m_writable_buf) return fk::core::Error::PermissionDenied;
  static constexpr size_t CAPACITY = 63;
  size_t to_copy = (size < CAPACITY) ? size : CAPACITY;
  size_t len = 0;
  while (len < to_copy && buffer[len] && buffer[len] != '\n') {
    m_writable_buf[len] = static_cast<char>(buffer[len]);
    ++len;
  }
  m_writable_buf[len] = '\0';
  return size;
}
