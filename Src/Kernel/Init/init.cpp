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
    using namespace FileSystem;
    using namespace Device;

    // TODO: Abstract this device inicialization in a unique algorithm
    auto& ata = ATAController::instance();
    auto& vfs = VFS::instance();

    auto* root = ramfs_create_unix_tree();
    FK::enforcef(root, "Failed to create RAMFS root");
    FK::enforcef(vfs.mount("/", root) == 0, "Failed to mount RAMFS on /");

    ata.initialize();

    auto* dev_dir = ramfs_lookup(root, "dev");
    if (!dev_dir) {
        FK::enforcef(root->ops && root->ops->mkdir, "Root vnode lacks mkdir op");
        FK::enforcef(root->ops->mkdir(root, "dev", 0755) == 0, "Failed to create /dev directory");
        dev_dir = ramfs_lookup(root, "dev");
        FK::enforcef(dev_dir, "/dev directory not found after creation");
    }

    for (int i = 0; i < ata.get_device_count(); ++i) {
        auto const* info = &ata.enumerate_devices()[i];
        if (!info->present)
            continue;

        char name_buf[8] = {};
        int drive_char = (info->drive == DriveType::Master) ? 0 : 1;
        LibC::snprintf(name_buf, sizeof(name_buf), "ata%d", drive_char);

        void* bd_mem = Falloc(sizeof(BlockDevice));
        auto* block_dev = new (bd_mem) BlockDevice {
            .name = name_buf,
            .block_size = 512,
            .block_count = 1024 * 1024, // TODO: pegar valor real do Identify
            .private_data = nullptr,
            .ops = nullptr // ou nullptr se não definido ainda
        };

        // BlockDeviceNode com placement new
        void* node_mem = Falloc(sizeof(BlockDeviceNode));
        auto* block_node = new (node_mem) BlockDeviceNode {
            .vnode = {},
            .device = block_dev,
            .channel = info->channel,
            .drive = info->drive
        };

        block_node->vnode.stat = {
            .size = block_dev->block_size * block_dev->block_count,
            .permissions = 0644,
            .inode = reinterpret_cast<LibC::uint64_t>(block_node),
            .uid = 0,
            .gid = 0,
            .type = VNodeType::Device
        };

        block_node->vnode.ops = nullptr;
        block_node->vnode.ref_count = 1;
        block_node->vnode.private_data = block_node;

        auto* res = vfs.create_device_node(
            dev_dir,
            block_dev->name,
            VNodeType::Device,
            block_node,
            block_node->vnode.ops);
        FK::enforcef(res != nullptr, "Failed to create device node %s", block_dev->name);
    }

    Log(LogLevel::INFO, "ATA devices registered in /dev successfully");
}
