#include <Kernel/Boot/init.h>
#include <LibFK/Algorithms/log.h>

#include <Kernel/FileSystem/RamFS/RamFS.h>

void init()
{
    klog("INIT", "Start init");

    ramfs_init();
}
