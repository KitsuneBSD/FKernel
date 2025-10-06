#pragma once

#include <LibFK/Container/static_vector.h>
#include <Kernel/FileSystem/Filesystem.h>

/**
 * @brief Singleton manager for registered filesystem drivers.
 *
 * Maintains a list of registered drivers and allows mounting devices into
 * the virtual filesystem.
 */
class FileSystemManager
{
private:
    FileSystemManager() = default;
    static_vector<FileSystemDriver *, 65535> m_filesystem_drivers;

public:
    /**
     * @brief Get the singleton instance of the filesystem manager.
     * @return Reference to the FileSystemManager instance
     */
    static FileSystemManager &the()
    {
        static FileSystemManager instance;
        return instance;
    }

    /**
     * @brief Register a new filesystem driver.
     * @param driver Pointer to the filesystem driver
     */
    void register_fs(FileSystemDriver *driver);

    /**
     * @brief Mount a filesystem on a mount point.
     * @param fs_name Name of the filesystem to use
     * @param device Pointer to the block device to mount
     * @param mount_point Name of the mount point (ignored internally)
     * @param root Root VNode where the mount point exists
     * @return Root VNode of the mounted filesystem, or nullptr on failure
     */
    RetainPtr<VNode> mount_fs(const char *fs_name, BlockDevice *device, const char *mount_point, RetainPtr<VNode> root);
};