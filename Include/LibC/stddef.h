#pragma once

namespace LibC {

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#if __cplusplus
#    define NULL nullptr
#else
#    define NULL (void*)0
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)

}
