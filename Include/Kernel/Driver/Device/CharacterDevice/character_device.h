#pragma once

#include <Kernel/Fs/Vfs/node.h>

namespace fkernel {

class CharacterDevice : public Node {
public:
    virtual ~CharacterDevice() override = default;

    virtual bool is_character_device() const override { return true; }

protected:
    CharacterDevice() = default;
};

} // namespace fkernel
