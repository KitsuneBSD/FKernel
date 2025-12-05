#pragma once

#include <LibC/assert.h>

#define assert(x) ASSERT(x)
#define ASSERT_NOT_REACHED ASSERT(false)

inline void not_implemented_yet() { assert(false); }
