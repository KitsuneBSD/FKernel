#include <Kernel/Boot/init.h>
#include <Kernel/FileSystem/vfs.h>
#include <LibFK/Algorithms/log.h>

void init() {
    klog("INIT", "Start init");

    VFS::init();
}