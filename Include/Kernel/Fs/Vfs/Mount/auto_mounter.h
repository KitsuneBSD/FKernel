#pragma once

#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>

namespace fkernel {

class AutoMounter {
public:
    static void mount_all_partitions();
    static void try_mount(fk::RefPtr<StorageDevice> device);
    static bool try_mount_at(fk::RefPtr<StorageDevice> device, const char* target_path);
};

}
