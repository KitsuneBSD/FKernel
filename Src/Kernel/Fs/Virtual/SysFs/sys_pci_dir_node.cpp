#include <Kernel/Fs/Virtual/SysFs/sys_pci_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_pci_dev_dir_node.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <LibFK/Utilities/memory.h>

// BDF format: "0000:00:1f.0" (domain:bus:device.function)
static void bdf_to_str(char* buf, size_t len, uint8_t bus, uint8_t dev, uint8_t func) {
  const char hex[] = "0123456789abcdef";
  if (len < 13) return;
  // "0000:bb:dd.f"
  buf[0] = '0'; buf[1] = '0'; buf[2] = '0'; buf[3] = '0'; buf[4] = ':';
  buf[5] = hex[(bus >> 4) & 0xf]; buf[6] = hex[bus & 0xf]; buf[7] = ':';
  buf[8] = hex[(dev >> 4) & 0xf]; buf[9] = hex[dev & 0xf]; buf[10] = '.';
  buf[11] = '0' + (func & 0x7);
  buf[12] = '\0';
}

fk::core::Result<void, fk::core::Error> SysPciDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& pci_dev : fkernel::PciManager::the().devices()) {
    char bdf[16];
    bdf_to_str(bdf, sizeof(bdf), pci_dev.address().bus(),
               pci_dev.address().device(), pci_dev.address().function());
    DirectoryEntry de;
    fk::memory::copy_n(de.name, bdf, sizeof(de.name));
    de.type = 1; // DT_DIR
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> SysPciDirNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  for (auto& pci_dev : fkernel::PciManager::the().devices()) {
    char bdf[16];
    bdf_to_str(bdf, sizeof(bdf), pci_dev.address().bus(),
               pci_dev.address().device(), pci_dev.address().function());
    if (fk::memory::compare(bdf, name) != 0) continue;
    return fk::RefPtr<Node>(fk::make_ref<SysPciDevDirNode>(
        pci_dev.vendor_id(), pci_dev.device_id(), pci_dev.class_code()).value());
  }
  return fk::core::Error::NotFound;
}
