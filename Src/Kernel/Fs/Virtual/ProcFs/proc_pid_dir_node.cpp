#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/ProcFs/proc_pid_dir_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_cmdline_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_exe_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_fd_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_maps_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_sched_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_stat_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_process_node.h>

using namespace fk::core;

fk::core::Result<void, fk::core::Error> ProcPidDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  const char* names[] = { "status", "stat", "cmdline", "sched", "exe", "fd", "maps" };
  for (auto* n : names) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, n, sizeof(de.name));
    de.type = 0;
    TRY(entries.push_back(de));
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
  if (fk::memory::compare(name, "sched") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcPidSchedNode>(m_pid).value());
  if (fk::memory::compare(name, "exe") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcPidExeNode>(m_pid).value());
  if (fk::memory::compare(name, "fd") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcPidFdDirNode>(m_pid).value());
  if (fk::memory::compare(name, "maps") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcPidMapsNode>(m_pid).value());
  return fk::core::Error::NotFound;
}
