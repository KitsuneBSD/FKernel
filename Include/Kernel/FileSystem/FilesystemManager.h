#pragma once

#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/Block/PartitionManager.h>
#include <Kernel/Driver/Ata/AtaController.h> // To get the list of block devices
#include <Kernel/FileSystem/VirtualFS/filesystem.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>

namespace fkernel::fs {

/**
 * @brief Manages the detection, initialization, and mounting of filesystems
 *        on detected block devices and partitions.
 *
 * This class follows the singleton pattern and centralizes the logic
 * for iterating through registered filesystem drivers and attempting to
 * probe and mount them on available partitions.
 */
class FilesystemManager {
private:
    FilesystemManager() = default; // Private constructor for singleton

    // Stores successfully mounted filesystems
    fk::containers::static_vector<fk::memory::RetainPtr<Filesystem>, 8> m_mounted_filesystems;

public:
    /**
     * @brief Get the singleton instance of FilesystemManager.
     * @return Reference to the global FilesystemManager instance.
     */
    static FilesystemManager &the();

    /**
     * @brief Initializes the FilesystemManager.
     *
     * This method iterates through all detected block devices (from AtaController),
     * identifies partitions using PartitionManager, and then attempts to probe
     * and mount filesystems on these partitions using registered filesystem drivers.
     */
    void initialize();

    /**
     * @brief Returns a reference to the list of currently mounted filesystems.
     * @return A const reference to the static_vector of mounted filesystems.
     */
    const fk::containers::static_vector<fk::memory::RetainPtr<Filesystem>, 8>& mounted_filesystems() const {
        return m_mounted_filesystems;
    }
};

} // namespace fkernel::fs
