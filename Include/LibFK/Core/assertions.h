#pragma once

#include <LibC/assert.h>

#define ASSERT_NOT_REACHED ASSERT(false)

inline void not_implemented_yet() { assert(false); }
