#pragma once 

#include <LibFK/Types/types.h>

enum class TaskState : uint8_t {
    Running,
    Ready,
    Blocked,
    Sleeping,
    Stopped,
    Zombie,
};

