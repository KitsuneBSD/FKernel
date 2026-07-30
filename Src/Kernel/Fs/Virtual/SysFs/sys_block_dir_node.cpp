#include <Kernel/Fs/Virtual/SysFs/sys_block_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_block_dev_dir_node.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Device/BlockDevice/block_device.h>
#include <LibFK/Utilities/memory.h>

fk::core::Result<void, fk::core::Error> SysBlockDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& dev : fkernel::DriverManager::the().devices()) {
    if (!dev || !dev->is_block_device()) continue;
    DirectoryEntry de;
    fk::memory::copy_n(de.name, dev->name().c_str(), sizeof(de.name) - 1);
    de.name[sizeof(de.name) - 1] = '\0';
    de.type = 1; // DT_DIR
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> SysBlockDirNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  for (auto& dev : fkernel::DriverManager::the().devices()) {
    if (!dev || !dev->is_block_device()) continue;
    if (fk::memory::compare(dev->name().c_str(), name) != 0) continue;
    auto* blk = static_cast<fkernel::BlockDevice*>(dev.get());
    uint64_t size_bytes = blk->sector_count().value() * blk->sector_size().value();
    uint32_t sector_sz = static_cast<uint32_t>(blk->sector_size().value());
    return fk::RefPtr<Node>(fk::make_ref<SysBlockDevDirNode>(name, size_bytes, sector_sz).value());
  }
  return fk::core::Error::NotFound;
}
