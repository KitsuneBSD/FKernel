#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Hardware/Cpu/cpu_context.h>

struct TaskContext {
  CpuContext registers;
  uint64_t stack_pointer;
  uint64_t kernel_stack_top;
  uint64_t user_rsp{0};
  uint64_t saved_rip{0};
  uint64_t saved_rflags{0};
  uint64_t fs_base{0};
  uint64_t gs_base{0};
  alignas(16) uint8_t fx_state[512]{};
  uint8_t* xsave_area{nullptr};
  size_t   xsave_size{0};
};

inline void* get_fpu_save_area(TaskContext& ctx) {
  if (ctx.xsave_area)
    return ctx.xsave_area;
  return ctx.fx_state;
}
