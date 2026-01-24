#pragma once

#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Driver/Storage/storage_device.h>

namespace fkernel {

class AutoMounter {
public:
    static void mount_all_partitions();
    static void try_mount(fk::RefPtr<StorageDevice> device);
};

}
