#include <Kernel/Fs/Virtual/SysFs/sys_devices_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_pci_dir_node.h>
#include <LibFK/Utilities/memory.h>

fk::core::Result<void, fk::core::Error> SysDevicesDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  DirectoryEntry de;
  fk::memory::copy_n(de.name, "pci", sizeof(de.name));
  de.type = 1;
  entries.push_back(de);
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> SysDevicesDirNode::lookup(const char* name) {
  if (fk::memory::compare(name, "pci") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysPciDirNode>().value());
  return fk::core::Error::NotFound;
}
