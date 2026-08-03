#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class Driver {
public:
    virtual ~Driver() = default;
    virtual const char* name() const = 0;
    virtual void probe() = 0;
};

} // namespace fkernel
