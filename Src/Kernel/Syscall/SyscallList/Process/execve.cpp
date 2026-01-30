#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>

extern "C" {
uint64_t sys_execve(uint64_t path_ptr, uint64_t argv_ptr, uint64_t envp_ptr,
                    uint64_t, uint64_t, uint64_t, PtRegs* regs) {
  if (!path_ptr) return -static_cast<int>(fk::core::Error::InvalidParameter);

  auto *task = SchedulerManager::the().current();
  if (!task)
    return -1;

  // 1. Capture data from user space before we lose the address space
  fk::text::String path = reinterpret_cast<const char *>(path_ptr);

  fk::containers::Vector<fk::text::String> args;
  if (argv_ptr) {
    char **user_argv = reinterpret_cast<char **>(argv_ptr);
    while (*user_argv) {
      args.push_back(*user_argv);
      user_argv++;
    }
  }

  fk::containers::Vector<fk::text::String> envs;
  if (envp_ptr) {
    char **user_envp = reinterpret_cast<char **>(envp_ptr);
    while (*user_envp) {
      envs.push_back(*user_envp);
      user_envp++;
    }
  }

  auto res = fkernel::VirtualFileSystem::the().open(path.c_str(), 0);
  if (res.is_error()) {
    fk::algorithms::kwarn("SYSCALL", "sys_execve: Failed to open %s: %d", path.c_str(), (int)res.error());
    return fkernel::return_error(res.error());
  }
  
  auto node = res.value()->node();
  fk::algorithms::klog("SYSCALL", "sys_execve: Opened %s, type=%s, size=%zu", 
                       path.c_str(), node->is_directory() ? "dir" : "file", node->size());

  if (node->is_directory())
    return fkernel::return_error(fk::core::Error::IsDirectory);

  // Check for shebang (#!)
  char magic[2];
  auto magic_read_res = node->read(0, 2, reinterpret_cast<uint8_t*>(magic));
  if (magic_read_res.is_ok() && magic_read_res.value() == 2 && magic[0] == '#' && magic[1] == '!') {
      fk::algorithms::klog("SYSCALL", "sys_execve: Detected script (shebang), invoking /bin/sh");
      
      fk::containers::Vector<fk::text::String> script_args;
      script_args.push_back("/bin/sh");
      script_args.push_back(path);
      for (size_t i = 1; i < args.size(); ++i) {
          script_args.push_back(args[i]);
      }
      
      auto sh_res = fkernel::VirtualFileSystem::the().open("/bin/sh", 0);
      if (sh_res.is_error()) {
          fk::algorithms::kwarn("SYSCALL", "sys_execve: Script detected but /bin/sh not found!");
          return fkernel::return_error(fk::core::Error::NotFound);
      }
      
      path = "/bin/sh";
      args = fk::types::move(script_args);
      node = sh_res.value()->node();
  }

  // 2. Load the new binary into a fresh address space
  uintptr_t new_cr3 = VirtualMemoryManager::the().create_address_space();
  VirtualMemoryManager::the().switch_address_space(new_cr3);
  task->resources.memory.cr3 = new_cr3;

  // If we are a vfork child, unblock the parent now that we have our own address space
  if (task->control.lifecycle.vfork_parent_id.is_valid()) {
      auto* parent = SchedulerManager::the().find_task(task->control.lifecycle.vfork_parent_id);
      if (parent && parent->control.lifecycle.vfork_waiting) {
          parent->control.lifecycle.vfork_waiting = false;
          SchedulerManager::the().wake_task(parent);
      }
      task->control.lifecycle.vfork_parent_id = fk::ProcessId();
  }

  auto entry_res = fkernel::ElfLoader::load(node);
  if (entry_res.is_error())
    return fkernel::return_error(entry_res.error());
  
  fkernel::ElfLoadResult elf_res = entry_res.value();
  uintptr_t entry = elf_res.entry;

  // 3. Setup the user stack (32KB)
  constexpr uintptr_t USER_STACK_TOP = 0x7fffffffe000;
  for (uintptr_t v = USER_STACK_TOP - 0x8000; v < USER_STACK_TOP; v += 0x1000) {
    uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
    VirtualMemoryManager::the().map_page(
        v, phys, PageFlags::Present | PageFlags::Writable | PageFlags::User);
    memset(reinterpret_cast<void *>(v), 0, 0x1000);
  }

  uintptr_t current_user_stack = USER_STACK_TOP;
  auto push_string = [&](const fk::text::String &s) -> uintptr_t {
    size_t len = s.length() + 1;
    current_user_stack -= len;
    memcpy(reinterpret_cast<void *>(current_user_stack), s.c_str(), len);
    return current_user_stack;
  };

  fk::containers::Vector<uintptr_t> arg_ptrs;
  for (size_t i = 0; i < args.size(); ++i) {
    arg_ptrs.push_back(push_string(args[i]));
  }

  fk::containers::Vector<uintptr_t> env_ptrs;
  for (size_t i = 0; i < envs.size(); ++i) {
    env_ptrs.push_back(push_string(envs[i]));
  }

  uintptr_t execfn_ptr = push_string(path);
  current_user_stack -= 16;
  uintptr_t random_ptr = current_user_stack;
  for (int i = 0; i < 16; ++i)
    reinterpret_cast<uint8_t *>(random_ptr)[i] = (uint8_t)(i ^ 0x77);

  // 4. Construct the pointer array below the strings
  current_user_stack &= ~0xFULL; // Alignment
  constexpr size_t auxv_pairs = 10;
  size_t total_ptrs = 1 + args.size() + 1 + envs.size() + 1 + (auxv_pairs * 2);
  if (total_ptrs % 2 != 0)
    total_ptrs++; // 16-byte alignment requirement

  current_user_stack -= (total_ptrs * sizeof(uintptr_t));
  current_user_stack &= ~0xFULL;

  uintptr_t *stack_ptr = reinterpret_cast<uintptr_t *>(current_user_stack);
  size_t idx = 0;

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
  stack_ptr[idx++] = 0;
  stack_ptr[idx++] = 0; // AT_NULL

  uintptr_t final_rsp = current_user_stack;

  // 5. Update return state via PtRegs and CpuControlBlock
  regs->rip = entry;
  regs->rsp = final_rsp;
  regs->rflags = 0x202; // IF | Reserved

  extern CpuControlBlock g_cpu_block;
  g_cpu_block.saved_rip = entry;
  g_cpu_block.user_rsp = final_rsp;
  g_cpu_block.saved_rflags = 0x202;

  task->resources.context.user_rsp = final_rsp;
  task->resources.context.saved_rip = entry;
  task->resources.context.saved_rflags = 0x202;

  task->dump_file_descriptors();

  fk::algorithms::klog("SYSCALL",
                       "EXECVE: Success. Arg0='%s', RSP=%p, Entry=%p",
                       args.size() > 0 ? args[0].c_str() : "N/A",
                       (void *)final_rsp, (void *)entry);

  return 0;
}
}