#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/Core/boot_info.h>
#include <Kernel/Fs/Virtual/DevFs/dev_fs.h>
#include <Kernel/Fs/Disk/RamDisk/ram_disk.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Sync/task_entries.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>

extern "C" void enter_user_mode(uintptr_t rip, uintptr_t rsp);

using namespace fkernel;

static bool setup_initial_filesystem() {
  auto& modules = boot::BootInfo::the().get_modules();
  if (modules.is_empty()) {
    fk::algorithms::kfatal("INIT", "No RamDisk module found!");
    return false;
  }

  // Assume the first module is our RamDisk
  auto& ramdisk_mod = modules[0];
  auto ramdisk_res = RamDiskNode::create(ramdisk_mod.start, ramdisk_mod.end);
  if (ramdisk_res.is_error()) {
    fk::algorithms::kfatal("INIT", "Failed to create RamDisk");
    return false;
  }

  auto ramdisk = ramdisk_res.value();
  // Mount RamDisk at root (read-only base), TmpFs is already mounted as root (writable overlay)
  // This way: writes go to TmpFs, reads check TmpFs first then fall back to RamDisk
  VirtualFileSystem::the().mount("/", ramdisk);

  // Remount DevFs to ensure /dev is populated in the new root
  auto& devfs = DevFs::the();
  VirtualFileSystem::the().mount("/dev", fk::RefPtr<Node>(&devfs));
  return true;
}

static bool setup_initial_file_descriptors(Task* current_task) {
  auto console_res = VirtualFileSystem::the().open("/dev/tty0", O_RDWR);
  if (console_res.is_error()) {
    fk::algorithms::kerror("INIT", "Failed to open /dev/tty0 for init process!");
    return false;
  }

  current_task->add_file_descriptor(console_res.value()); // 0: stdin
  current_task->add_file_descriptor(console_res.value()); // 1: stdout
  current_task->add_file_descriptor(console_res.value()); // 2: stderr
  return true;
}

static fk::core::Result<ElfLoadResult, fk::core::Error>
load_init_executable_and_setup_address_space() {
  const char* init_path = boot::BootInfo::the().get_init_path();
  auto init_dentry_res = VirtualFileSystem::the().resolve_path(init_path);
  if (init_dentry_res.is_error()) {
    fk::algorithms::kerror("INIT", "Could not find %s in VFS", init_path);
    return init_dentry_res.error();
  }

  uintptr_t new_cr3 = VirtualMemoryManager::the().create_address_space();
  VirtualMemoryManager::the().switch_address_space(new_cr3);
  SchedulerManager::the().current()->resources.memory.cr3 = new_cr3;

  return ElfLoader::load(init_dentry_res.value()->top_node());
}

static uintptr_t setup_user_stack() {
  // Map user stack (32 KB) at the top of the user address space
  constexpr uintptr_t USER_STACK_TOP = 0x7fffffffe000;
  constexpr size_t STACK_PAGES = 8;
  for (size_t i = 0; i < STACK_PAGES; ++i) {
    uintptr_t stack_phys = PhysicalMemoryManager::the().alloc_page();
    // Zero through the direct-map alias: a raw write to the user VA would be a
    // kernel SMAP violation (kernel write to a user page, AC=0).
    fk::memory::set(reinterpret_cast<void*>(stack_phys + KERNEL_VIRT_BASE), 0, 0x1000);
    VirtualMemoryManager::the().map_page(USER_STACK_TOP - (i + 1) * 0x1000, stack_phys,
                                         PageFlags::Present | PageFlags::Writable |
                                             PageFlags::User);
  }
  return USER_STACK_TOP;
}

static void setup_initial_stack_frame_and_enter_user_mode(uintptr_t entry, uintptr_t user_stack_top,
                                                          const ElfLoadResult& elf_res) {
  // Setup initial stack frame for Musl/BusyBox. The frame lives in the mapped user
  // stack, so build it inside an SMAP window (arch_smap_begin/end), matching the
  // loader's copy_segment_data() pattern.
  arch_smap_begin();
  char* string_area = reinterpret_cast<char*>(user_stack_top) - 128;
  fk::memory::copy_string(string_area, boot::BootInfo::the().get_init_path());
  fk::memory::copy_string(string_area + 32, "PATH=/bin:/sbin:/usr/bin:/usr/sbin");
  uintptr_t argv0_addr = user_stack_top - 128;
  uintptr_t envp0_addr = user_stack_top - 96;

  // Space for auxv: allocate 25 slots then align RSP to 16 bytes.
  // System V ABI: RSP at _start must be 16-byte aligned so that after
  // `call main` (which pushes 8 bytes) + `push rbp`, movaps targets are aligned.
  uintptr_t raw_ptr = reinterpret_cast<uintptr_t>(reinterpret_cast<uintptr_t*>(string_area) - 25);
  uintptr_t* pointers = reinterpret_cast<uintptr_t*>(raw_ptr & ~(uintptr_t)15);
  size_t idx = 0;
  pointers[idx++] = 1;          // argc
  pointers[idx++] = argv0_addr; // argv[0]
  pointers[idx++] = 0;          // argv[1] (NULL)
  pointers[idx++] = envp0_addr; // envp[0]
  pointers[idx++] = 0;          // envp[1] (NULL)

  // Auxv
  pointers[idx++] = 3;
  pointers[idx++] = elf_res.ph_addr; // AT_PHDR
  pointers[idx++] = 4;
  pointers[idx++] = elf_res.ph_ent; // AT_PHENT
  pointers[idx++] = 5;
  pointers[idx++] = elf_res.ph_num; // AT_PHNUM
  pointers[idx++] = 6;
  pointers[idx++] = PAGE_SIZE; // AT_PAGESZ
  pointers[idx++] = 0;
  pointers[idx++] = 0; // AT_NULL

  uintptr_t final_rsp = reinterpret_cast<uintptr_t>(pointers);
  arch_smap_end();

  enter_user_mode(entry, final_rsp);

  arch_halt_loop();
}

extern "C" void init_task_entry() {
  fk::algorithms::klog("INIT", "Init process trampoline started.");

  if (!setup_initial_filesystem()) {
    fk::algorithms::kfatal("INIT", "Failed to setup initial filesystem.");
  }

  auto* current = SchedulerManager::the().current();
  if (!setup_initial_file_descriptors(current)) {
    fk::algorithms::kfatal("INIT", "Failed to setup initial file descriptors.");
  }

  auto elf_load_res = load_init_executable_and_setup_address_space();
  if (elf_load_res.is_error()) {
    fk::algorithms::kfatal("INIT", "Failed to load %s ELF (Error %d)",
                           boot::BootInfo::the().get_init_path(),
                           (int)elf_load_res.error());
  }
  ElfLoadResult elf_res = elf_load_res.value();

  current->set_heap_regions(elf_res.highest_load_end, elf_res.highest_load_end);
  current->set_mmap_regions(0x40000000, 0x40000000);

  uintptr_t entry = elf_res.entry;
  fk::algorithms::klog("INIT", "Jumping to user-mode init at %p", (void*)entry);

  uintptr_t user_stack_top = setup_user_stack();
  setup_initial_stack_frame_and_enter_user_mode(entry, user_stack_top, elf_res);
}
