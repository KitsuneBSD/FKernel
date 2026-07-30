#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Text/fixed_string.h>

// /sys/devices/pci/<BDF>/ — per-PCI-device directory with vendor/device/class.
class SysPciDevDirNode : public Node {
public:
  SysPciDevDirNode(uint16_t vendor, uint16_t device_id, uint8_t class_code);
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return true; }
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
private:
  fk::text::fixed_string<8> m_vendor;
  fk::text::fixed_string<8> m_device;
  fk::text::fixed_string<8> m_class;
};
