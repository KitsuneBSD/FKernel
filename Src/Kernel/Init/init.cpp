#include <Kernel/Boot/init.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/Posix/unistd.h>
#include <LibFK/Algorithms/log.h>

void init() {
    klog("INIT", "Start init");

    VFS::init();
    fdtable_init();
}
