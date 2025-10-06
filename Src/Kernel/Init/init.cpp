#include <Kernel/Boot/init.h>
#include <Kernel/FileSystem/FilesystemManager.h>
#include <LibFK/Algorithms/log.h>

void init()
{
    klog("INIT", "Start init");
    auto fs_manager = FileSystemManager::the();
}
