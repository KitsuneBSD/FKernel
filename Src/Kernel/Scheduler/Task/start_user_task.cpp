#include <LibFK/Types/types.h>

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>

extern "C" void enter_user_mode(uint64_t user_rip, uint64_t user_rsp);

extern "C" void start_user_task(uint64_t user_rip, uint64_t user_rsp) {
  enter_user_mode(user_rip, user_rsp);
  arch_halt_loop();
}
