#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Synchronization/interrupt_disabler.h>
#include <LibFK/Text/string.h>

#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/DebugFs/debug_fs.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Posix/signal_defs.h>

extern "C" {
uint64_t sys_execve(uint64_t path_ptr, uint64_t argv_ptr, uint64_t envp_ptr, uint64_t, uint64_t,
                    uint64_t, PtRegs* regs) {
  if (!path_ptr || !fkernel::memory::is_user_address(path_ptr, 1))
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  auto* task = SchedulerManager::the().current();
  if (!task)
    return -1;

  // 1. Capture data from user space before we lose the address space
  char kpath[512];
  {
    size_t len = 0;
    for (; len < sizeof(kpath) - 1; ++len) {
      char c;
      if (fkernel::memory::copy_from_user(&c, reinterpret_cast<const void*>(path_ptr + len), 1).is_error())
        return -14;
      kpath[len] = c;
      if (c == '\0') break;
    }
    kpath[sizeof(kpath) - 1] = '\0';
  }
  fk::text::String path(kpath);

  // Read argv and envp via copy_from_user — never dereference user pointers directly.
  constexpr size_t MAX_ARGS = 1024;
  constexpr size_t MAX_ARG_LEN = 4096;

  auto read_user_string_array = [&](uint64_t user_array_ptr, fk::containers::Vector<fk::text::String>& out) {
    if (!user_array_ptr || !fkernel::memory::is_user_address(user_array_ptr, sizeof(char*)))
      return;
    // Read one pointer at a time to avoid bulk-copying past the end of a mapped page.
    for (size_t i = 0; i < MAX_ARGS; ++i) {
      uint64_t entry_addr = user_array_ptr + i * sizeof(char*);
      if (!fkernel::memory::is_user_address(entry_addr, sizeof(char*))) break;
      char* uptr = nullptr;
      auto ptr_res = fkernel::memory::copy_from_user(&uptr, reinterpret_cast<const void*>(entry_addr), sizeof(char*));
      if (ptr_res.is_error()) break;
      if (!uptr) break;
      if (!fkernel::memory::is_user_address(reinterpret_cast<uint64_t>(uptr), 1)) break;
      char buf[MAX_ARG_LEN];
      size_t len = 0;
      bool scan_ok = true;
      for (; len < MAX_ARG_LEN - 1; ++len) {
        char c;
        if (fkernel::memory::copy_from_user(&c, reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(uptr) + len), 1).is_error()) {
          scan_ok = false;
          break;
        }
        buf[len] = c;
        if (c == '\0') break;
      }
      buf[MAX_ARG_LEN - 1] = '\0';
      if (!scan_ok) break;
      TRY_OR_FATAL(out.push_back(fk::text::String(buf)));
    }
  };

  fk::containers::Vector<fk::text::String> args;
  read_user_string_array(argv_ptr, args);

  fk::containers::Vector<fk::text::String> envs;
  read_user_string_array(envp_ptr, envs);

  auto res = fkernel::VirtualFileSystem::the().open(path.c_str(), 0);
  if (res.is_error()) {
    fk::algorithms::kwarn("EXEC", "execve: VFS open failed for %s", path.c_str());
    return fkernel::return_error(res.error());
  }

  auto node = res.value()->node();

  if (node->is_directory())
    return fkernel::return_error(fk::core::Error::IsDirectory);

  // Check for shebang (#!)
  char magic[2];
  auto magic_read_res = node->read(0, 2, reinterpret_cast<uint8_t*>(magic));
  if (magic_read_res.is_ok() && magic_read_res.value() == 2 && magic[0] == '#' && magic[1] == '!') {

    fk::containers::Vector<fk::text::String> script_args;
    TRY_OR_FATAL(script_args.push_back("/bin/sh"));
    TRY_OR_FATAL(script_args.push_back(path));
    for (size_t i = 1; i < args.size(); ++i) {
      TRY_OR_FATAL(script_args.push_back(args[i]));
    }

    auto sh_res = fkernel::VirtualFileSystem::the().open("/bin/sh", 0);
    if (sh_res.is_error()) {

      return fkernel::return_error(fk::core::Error::NotFound);
    }

    path = "/bin/sh";
    args = fk::types::move(script_args);
    node = sh_res.value()->node();
  }

  // execve in a multi-threaded process: terminate all sibling threads first
  {
    auto self_tgid = task->control.identity.tgid;
    auto self_pid  = task->control.identity.id;
    uint64_t max_pid = SchedulerManager::the().last_pid().value();
    for (uint64_t probe = 1; probe <= max_pid; ++probe) {
      if (fk::ProcessId(probe) == self_pid) continue;
      auto sibling = SchedulerManager::the().find_task(fk::ProcessId(probe));
      if (!sibling) continue;
      if (sibling->control.identity.tgid != self_tgid) continue;
      fkernel::ipc::SignalDelivery::send_signal(sibling.get(), SIGKILL);
    }
  }

  // 2. Load the new binary into a fresh address space.
  uintptr_t old_cr3 = task->resources.memory.cr3;
  uintptr_t new_cr3;
  {
    // Atomic: scheduler must never see task->cr3 and hardware CR3 disagree.
    fk::synchronization::ScopedInterruptDisabler no_preempt;
    new_cr3 = VirtualMemoryManager::the().create_address_space();
    bool shared_cr3 = task->control.lifecycle.is_vfork_sharing_address_space;
    task->control.lifecycle.is_vfork_sharing_address_space = false;
    task->resources.memory.prev_cr3 = shared_cr3 ? 0 : old_cr3;
    task->resources.memory.cr3 = new_cr3;
    if (task->resources.memory.shadow_cr3) {
      PhysicalMemoryManager::the().free_page(task->resources.memory.shadow_cr3);
      task->resources.memory.shadow_cr3 = 0;
    }
    VirtualMemoryManager::the().switch_address_space(new_cr3);
  }

  // If we are a vfork child, unblock the parent now that we have our own address space
  if (task->control.lifecycle.vfork_parent_id.is_valid()) {
    auto parent = SchedulerManager::the().find_task(task->control.lifecycle.vfork_parent_id);
    if (parent && parent->control.lifecycle.vfork_waiting) {
      parent->control.lifecycle.vfork_waiting = false;
      SchedulerManager::the().wake_task(parent.get());
    }
    task->control.lifecycle.vfork_parent_id = fk::ProcessId();
  }

  auto entry_res = fkernel::ElfLoader::load(node);
  if (entry_res.is_error()) {
    fk::algorithms::kwarn("EXEC", "execve: ELF load failed");
    return fkernel::return_error(entry_res.error());
  }

  fkernel::ElfLoadResult elf_res = entry_res.value();
  uintptr_t entry = elf_res.entry;
  fkernel::notify_proc_kqueue(task, fkernel::NOTE_EXEC);
  fk::algorithms::klog("EXEC", "execve: loading %s, entry=%p", path.c_str(), (void*)entry);


  task->resources.files.exe_path = path.c_str();
  task->control.identity.name = path.c_str();

  task->set_heap_regions(elf_res.highest_load_end, elf_res.highest_load_end);
  task->set_mmap_regions(0x40000000, 0x40000000);

  // POSIX: Reset caught signal handlers to SIG_DFL across exec.
  //        SIG_IGN handlers are preserved. Signal mask is preserved (not cleared).
  for (int i = 1; i < NSIG; ++i) {
    if (task->resources.ipc.signals.actions[i].sa_handler != SIG_IGN) {
      fk::memory::set(&task->resources.ipc.signals.actions[i], 0,
                       sizeof(struct sigaction));
    }
  }
  task->resources.ipc.signals.pending = 0;

  // 2.5 Setup TLS if PT_TLS is present (x86-64 variant II)
  uint64_t tls_fs_base = 0;
  if (elf_res.tls.present && elf_res.tls.memsz > 0) {
    size_t tls_pages = (elf_res.tls.memsz + sizeof(uintptr_t) + 0xFFF) / 0x1000;
    constexpr uintptr_t TLS_VIRT_BASE = 0x7FFFFE000000ULL;
    for (size_t i = 0; i < tls_pages; ++i) {
      uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
      fk::memory::set(reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0, 0x1000);
      VirtualMemoryManager::the().map_page(TLS_VIRT_BASE + i * 0x1000, phys,
                                           PageFlags::Present | PageFlags::Writable |
                                               PageFlags::User);
    }
    if (CPU::the().has_smap()) arch_smap_begin();
    fk::memory::copy(reinterpret_cast<void*>(TLS_VIRT_BASE),
           reinterpret_cast<const void*>(elf_res.tls.vaddr), elf_res.tls.filesz);
    uintptr_t tp = TLS_VIRT_BASE + elf_res.tls.memsz;
    *reinterpret_cast<uintptr_t*>(tp) = tp; // self-reference
    if (CPU::the().has_smap()) arch_smap_end();
    tls_fs_base = tp;
    task->resources.context.fs_base = tp;
    CPU::the().write_msr(MSR_FS_BASE, tp);
  }

  // 3. Setup the user stack (32KB); enforce NX unless PT_GNU_STACK marks it executable
  constexpr uintptr_t USER_STACK_TOP = 0x7fffffffe000;
  PageFlags stack_flags = PageFlags::Present | PageFlags::Writable | PageFlags::User;
  if (!elf_res.stack_executable)
    stack_flags = stack_flags | PageFlags::ExecuteDisable;
  for (uintptr_t v = USER_STACK_TOP - 0x8000; v < USER_STACK_TOP; v += 0x1000) {
    uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
    // Zero through the kernel direct map BEFORE the page is mapped as User;
    // a raw write through the user mapping violates SMAP (AC=0) and faults.
    fk::memory::set(reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0, 0x1000);
    VirtualMemoryManager::the().map_page(v, phys, stack_flags);
  }

  uintptr_t current_user_stack = USER_STACK_TOP;
  auto push_string = [&](const fk::text::String& s) -> uintptr_t {
    size_t len = s.length() + 1;
    current_user_stack -= len;
    fkernel::memory::copy_to_user(reinterpret_cast<void*>(current_user_stack), s.c_str(), len);
    return current_user_stack;
  };

  fk::containers::Vector<uintptr_t> arg_ptrs;
  for (size_t i = 0; i < args.size(); ++i) {
    TRY_OR_FATAL(arg_ptrs.push_back(push_string(args[i])));
  }

  fk::containers::Vector<uintptr_t> env_ptrs;
  for (size_t i = 0; i < envs.size(); ++i) {
    TRY_OR_FATAL(env_ptrs.push_back(push_string(envs[i])));
  }

  uintptr_t execfn_ptr = push_string(path);
  current_user_stack -= 16;
  uintptr_t random_ptr = current_user_stack;
  uint8_t random_bytes[16];
  for (int i = 0; i < 16; ++i)
    random_bytes[i] = (uint8_t)(i ^ 0x77);
  fkernel::memory::copy_to_user(reinterpret_cast<void*>(random_ptr), random_bytes,
                                sizeof(random_bytes));

  // 4. Construct the pointer array below the strings
  current_user_stack &= ~0xFULL; // Alignment
  size_t auxv_pairs = tls_fs_base ? 11u : 10u;
  size_t total_ptrs = 1 + args.size() + 1 + envs.size() + 1 + (auxv_pairs * 2);
  if (total_ptrs % 2 != 0)
    total_ptrs++; // 16-byte alignment requirement

  current_user_stack -= (total_ptrs * sizeof(uintptr_t));
  current_user_stack &= ~0xFULL;

  uintptr_t* stack_ptr = reinterpret_cast<uintptr_t*>(current_user_stack);
  size_t idx = 0;

  if (CPU::the().has_smap()) arch_smap_begin();

  stack_ptr[idx++] = args.size();
  for (size_t i = 0; i < arg_ptrs.size(); ++i)
    stack_ptr[idx++] = arg_ptrs[i];
  stack_ptr[idx++] = 0; // argv NULL

  for (size_t i = 0; i < env_ptrs.size(); ++i)
    stack_ptr[idx++] = env_ptrs[i];
  stack_ptr[idx++] = 0; // envp NULL

  // Auxv (Critical for Musl LibC)
  stack_ptr[idx++] = 3;
  stack_ptr[idx++] = elf_res.ph_addr; // AT_PHDR
  stack_ptr[idx++] = 4;
  stack_ptr[idx++] = elf_res.ph_ent; // AT_PHENT
  stack_ptr[idx++] = 5;
  stack_ptr[idx++] = elf_res.ph_num; // AT_PHNUM
  stack_ptr[idx++] = 6;
  stack_ptr[idx++] = 4096; // AT_PAGESZ
  stack_ptr[idx++] = 11;
  stack_ptr[idx++] = 0; // AT_UID
  stack_ptr[idx++] = 13;
  stack_ptr[idx++] = 0; // AT_GID
  stack_ptr[idx++] = 23;
  stack_ptr[idx++] = 0; // AT_SECURE
  stack_ptr[idx++] = 25;
  stack_ptr[idx++] = random_ptr; // AT_RANDOM
  stack_ptr[idx++] = 31;
  stack_ptr[idx++] = execfn_ptr; // AT_EXECFN
  if (tls_fs_base) {
    stack_ptr[idx++] = 51; // AT_MINSIGSTKSZ reused as AT_TLS base signal
    stack_ptr[idx++] = tls_fs_base;
  }
  stack_ptr[idx++] = 0;
  stack_ptr[idx++] = 0; // AT_NULL

  if (CPU::the().has_smap()) arch_smap_end();

  uintptr_t final_rsp = current_user_stack;

  // 5. Update return state via PtRegs and CpuControlBlock
  regs->rip = entry;
  regs->rsp = final_rsp;
  regs->rflags = 0x202; // IF | Reserved

  current_cpu_block().saved_rip = entry;
  current_cpu_block().user_rsp = final_rsp;
  current_cpu_block().saved_rflags = 0x202;

  task->resources.context.user_rsp = final_rsp;
  task->resources.context.saved_rip = entry;
  task->resources.context.saved_rflags = 0x202;

  // Close all FDs marked O_CLOEXEC across execve
  for (size_t i = 0; i < task->resources.files.descriptors.size(); ++i) {
    auto& desc = task->resources.files.descriptors[i];
    if (desc && desc->is_cloexec()) {
      if (task->resources.ipc.cspace && i < task->resources.files.cap_handles.size()) {
        uint32_t h = task->resources.files.cap_handles[i];
        if (h != fkernel::ipc::INVALID_HANDLE)
          task->resources.ipc.cspace->revoke_fd(h);
        task->resources.files.cap_handles[i] = fkernel::ipc::INVALID_HANDLE;
      }
      desc = nullptr;
    }
  }

  task->dump_file_descriptors();

  uintptr_t verify_phys = VirtualMemoryManager::the().translate(entry & ~0xFFFULL);
  fk::algorithms::klog("EXEC", "execve done: cr3=%p entry=%p phys@entry_page=%p",
                       (void*)task->resources.memory.cr3, (void*)entry, (void*)verify_phys);

  return 0;
}
}