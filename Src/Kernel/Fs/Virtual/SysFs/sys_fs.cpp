#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/SysFs/sys_fs.h>
#include <Kernel/Fs/Virtual/SysFs/sys_block_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_devices_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_static_dir_node.h>

fk::core::Result<void, fk::core::Error> SysFsNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  const char* subdirs[] = { "block", "class", "devices", "fs" };
  for (auto* s : subdirs) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, s, sizeof(de.name));
    de.type = 1; // DT_DIR
    TRY(entries.push_back(de));
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> SysFsNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  if (fk::memory::compare(name, "block") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysBlockDirNode>().value());
  if (fk::memory::compare(name, "devices") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysDevicesDirNode>().value());
  if (fk::memory::compare(name, "class") == 0 || fk::memory::compare(name, "fs") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysStaticDirNode>().value());
  return fk::core::Error::NotFound;
}
