#include <Kernel/Boot/init.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/FileSystem/RamFS/RamFS.h>
#include <Kernel/FileSystem/VFS/VirtualFileSystem.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

void init()
{
    ATAController::instance().initialize();

    auto& vfs = FileSystem::VFS::instance();
    auto* root = FileSystem::ramfs_create_unix_tree();

    FK::enforcef(root != nullptr, "Failed to create RAMFS root");

    int result = vfs.mount("/", root);
    FK::enforcef(result == 0, "Failed to mount RAMFS on /");

    FileSystem::VNode* home_dir = ramfs_lookup(root, "home");
    FK::enforcef(home_dir != nullptr, "Failed to lookup /home directory");

    Log(LogLevel::INFO, "RAMFS created with sucessfully");
}
