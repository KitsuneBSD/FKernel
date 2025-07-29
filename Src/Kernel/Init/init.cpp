#include "Kernel/FileSystem/DevFS/DevFS.h"
#include <Kernel/Boot/init.h>
#include <Kernel/Devices/Storage/BlockDeviceOperations.h>
#include <Kernel/Devices/Storage/BlockDeviceTypes.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaDevice.h>
#include <Kernel/Driver/Ata/AtaTypes.h>
#include <Kernel/FileSystem/RamFS/RamFS.h>
#include <Kernel/FileSystem/VFS/VirtualFileSystem.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>
#include <LibFK/new.h>

void init()
{
    auto devfs_root = FileSystem::devfs_init();
    auto ramfs_root = FileSystem::ramfs_create_unix_tree();

    FileSystem::VFS::instance().mount("/", ramfs_root);
    FileSystem::VFS::instance().mount("/dev", devfs_root);

    ATAController::instance().initialize();
}
