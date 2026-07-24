#include <Kernel/Fs/ProcFs/proc_pid_dir_node.h>
#include <Kernel/Fs/ProcFs/proc_process_node.h>
#include <Kernel/Fs/ProcFs/proc_pid_stat_node.h>
#include <Kernel/Fs/ProcFs/proc_pid_cmdline_node.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fk::core;

fk::core::Result<void, fk::core::Error> ProcPidDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  const char* names[] = { "status", "stat", "cmdline" };
  for (auto* n : names) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, n, sizeof(de.name));
    de.type = 0;
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> ProcPidDirNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  if (fk::memory::compare(name, "status") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcProcessNode>(m_pid).value());
  if (fk::memory::compare(name, "stat") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcPidStatNode>(m_pid).value());
  if (fk::memory::compare(name, "cmdline") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcPidCmdlineNode>(m_pid).value());
  return fk::core::Error::NotFound;
}
