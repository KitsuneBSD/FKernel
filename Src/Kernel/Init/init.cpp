#include "LibFK/log.h"
#include <Kernel/Boot/init.h>
#include <Kernel/FileSystem/RamFS/RamFS.h>
#include <Kernel/FileSystem/VFS/VirtualFileSystem.h>
#include <LibFK/enforce.h>

void init()
{
    auto& vfs = FileSystem::VFS::instance();
    auto* root = FileSystem::ramfs_create_root();

    FK::enforcef(root != nullptr, "Failed to create RAMFS root");

    int result = vfs.mount("/", root);
    FK::enforcef(result == 0, "Failed to mount RAMFS on /");

    int res = ramfs_mkdir(root, "home", 0755);
    FK::enforcef(res == 0, "Failed to create /home directory");

    FileSystem::VNode* home_dir = ramfs_lookup(root, "home");
    FK::enforcef(home_dir != nullptr, "Failed to lookup /home directory");

    Log(LogLevel::INFO, "RAMFS created with sucessfully");
}
