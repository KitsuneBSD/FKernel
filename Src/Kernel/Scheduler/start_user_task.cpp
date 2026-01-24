#include <LibFK/Types/types.h>

extern "C" void enter_user_mode(uint64_t user_rip, uint64_t user_rsp);

extern "C" void start_user_task(uint64_t user_rip, uint64_t user_rsp) {
  enter_user_mode(user_rip, user_rsp);
  for (;;)
    asm volatile("hlt");
}
