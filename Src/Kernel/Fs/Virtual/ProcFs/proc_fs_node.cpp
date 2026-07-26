#include <Kernel/Fs/Virtual/ProcFs/proc_fs_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_partitions_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_process_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_self_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_version_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_mounts_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_uptime_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_meminfo_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_stat_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_sys_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_loadavg_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_cpuinfo_node.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_pid_dir_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fk::core;

fk::core::Result<void, fk::core::Error> ProcFsNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  DirectoryEntry p_de;
  fk::memory::copy_n(p_de.name, "partitions", sizeof(p_de.name));
  p_de.type = 0;
  entries.push_back(p_de);

  DirectoryEntry self_de;
  fk::memory::copy_n(self_de.name, "self", sizeof(self_de.name));
  self_de.type = 0;
  entries.push_back(self_de);

  DirectoryEntry ver_de;
  fk::memory::copy_n(ver_de.name, "version", sizeof(ver_de.name));
  ver_de.type = 0;
  entries.push_back(ver_de);

  DirectoryEntry mnt_de;
  fk::memory::copy_n(mnt_de.name, "mounts", sizeof(mnt_de.name));
  mnt_de.type = 0;
  entries.push_back(mnt_de);

  DirectoryEntry uptime_de;
  fk::memory::copy_n(uptime_de.name, "uptime", sizeof(uptime_de.name));
  uptime_de.type = 0;
  entries.push_back(uptime_de);

  DirectoryEntry meminfo_de;
  fk::memory::copy_n(meminfo_de.name, "meminfo", sizeof(meminfo_de.name));
  meminfo_de.type = 0;
  entries.push_back(meminfo_de);

  DirectoryEntry stat_de;
  fk::memory::copy_n(stat_de.name, "stat", sizeof(stat_de.name));
  stat_de.type = 0;
  entries.push_back(stat_de);

  DirectoryEntry sys_de;
  fk::memory::copy_n(sys_de.name, "sys", sizeof(sys_de.name));
  sys_de.type = 1;
  entries.push_back(sys_de);

  DirectoryEntry loadavg_de;
  fk::memory::copy_n(loadavg_de.name, "loadavg", sizeof(loadavg_de.name));
  loadavg_de.type = 0;
  entries.push_back(loadavg_de);

  DirectoryEntry cpuinfo_de;
  fk::memory::copy_n(cpuinfo_de.name, "cpuinfo", sizeof(cpuinfo_de.name));
  cpuinfo_de.type = 0;
  entries.push_back(cpuinfo_de);

  auto& scheduler = SchedulerManager::the();
  uint64_t max_pid = scheduler.last_pid();
  for (uint64_t pid = 1; pid < max_pid; ++pid) {
    if (scheduler.find_task(fk::ProcessId(pid))) {
      DirectoryEntry de;
      char pid_str[32];
      snprintf(pid_str, sizeof(pid_str), "%lu", pid);
      fk::memory::copy_n(de.name, pid_str, sizeof(de.name));
      de.type = 0;
      entries.push_back(de);
    }
  }

  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> ProcFsNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;

  if (fk::memory::compare(name, "partitions") == 0) {
    auto res = fk::make_ref<ProcPartitionsNode>().value();
    if (!res) return fk::core::Error::OutOfMemory;
    return fk::RefPtr<Node>(res);
  }
  if (fk::memory::compare(name, "self") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSelfNode>().value());
  if (fk::memory::compare(name, "version") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcVersionNode>().value());
  if (fk::memory::compare(name, "mounts") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcMountsNode>().value());
  if (fk::memory::compare(name, "uptime") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcUptimeNode>().value());
  if (fk::memory::compare(name, "meminfo") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcMeminfoNode>().value());
  if (fk::memory::compare(name, "stat") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcStatNode>().value());
  if (fk::memory::compare(name, "sys") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcSysNode>().value());
  if (fk::memory::compare(name, "loadavg") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcLoadavgNode>().value());
  if (fk::memory::compare(name, "cpuinfo") == 0)
    return fk::RefPtr<Node>(fk::make_ref<ProcCpuinfoNode>().value());

  uint64_t pid = 0;
  const char* p = name;
  while (*p) {
    if (*p < '0' || *p > '9') return fk::core::Error::NotFound;
    pid = pid * 10 + static_cast<uint64_t>(*p - '0');
    ++p;
  }

  auto res = fk::make_ref<ProcPidDirNode>(pid).value();
  if (!res) return fk::core::Error::OutOfMemory;
  return fk::RefPtr<Node>(res);
}
