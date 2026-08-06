#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/SysFs/sys_pci_dev_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_attr_node.h>

static void u16_to_hex(char* buf, uint16_t val) {
  const char hex[] = "0123456789abcdef";
  buf[0] = '0'; buf[1] = 'x';
  buf[2] = hex[(val >> 12) & 0xf];
  buf[3] = hex[(val >>  8) & 0xf];
  buf[4] = hex[(val >>  4) & 0xf];
  buf[5] = hex[(val      ) & 0xf];
  buf[6] = '\0';
}

static void u8_to_hex(char* buf, uint8_t val) {
  const char hex[] = "0123456789abcdef";
  buf[0] = '0'; buf[1] = 'x';
  buf[2] = hex[(val >> 4) & 0xf];
  buf[3] = hex[(val     ) & 0xf];
  buf[4] = '\0';
}

SysPciDevDirNode::SysPciDevDirNode(uint16_t vendor, uint16_t device_id, uint8_t class_code) {
  char buf[8];
  u16_to_hex(buf, vendor);    m_vendor  = fk::text::fixed_string<8>(buf);
  u16_to_hex(buf, device_id); m_device  = fk::text::fixed_string<8>(buf);
  u8_to_hex(buf, class_code); m_class   = fk::text::fixed_string<8>(buf);
}

fk::core::Result<void, fk::core::Error> SysPciDevDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  const char* names[] = { "vendor", "device", "class" };
  for (auto* n : names) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, n, sizeof(de.name));
    de.type = 0;
    TRY(entries.push_back(de));
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> SysPciDevDirNode::lookup(const char* name) {
  if (fk::memory::compare(name, "vendor") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysAttrNode>(m_vendor.c_str()).value());
  if (fk::memory::compare(name, "device") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysAttrNode>(m_device.c_str()).value());
  if (fk::memory::compare(name, "class") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysAttrNode>(m_class.c_str()).value());
  return fk::core::Error::NotFound;
}
