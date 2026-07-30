#include <Kernel/Fs/Virtual/SysFs/sys_block_dev_dir_node.h>
#include <Kernel/Fs/Virtual/SysFs/sys_attr_node.h>
#include <LibFK/Utilities/memory.h>

static void u64_to_str(char* buf, size_t len, uint64_t val) {
  if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
  char tmp[24];
  int i = 0;
  uint64_t v = val;
  while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
  int j = 0;
  for (int k = i - 1; k >= 0 && (size_t)j + 1 < len; --k) buf[j++] = tmp[k];
  buf[j] = '\0';
}

SysBlockDevDirNode::SysBlockDevDirNode(const char* dev_name, uint64_t size_bytes, uint32_t sector_sz)
    : m_dev_name(dev_name)
{
  char buf[24];
  u64_to_str(buf, sizeof(buf), size_bytes);
  m_size_str = fk::text::fixed_string<24>(buf);
  u64_to_str(buf, sizeof(buf), static_cast<uint64_t>(sector_sz));
  m_sector_size_str = fk::text::fixed_string<12>(buf);
}

fk::core::Result<void, fk::core::Error> SysBlockDevDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  const char* names[] = { "size", "sector_size" };
  for (auto* n : names) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, n, sizeof(de.name));
    de.type = 0;
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> SysBlockDevDirNode::lookup(const char* name) {
  if (fk::memory::compare(name, "size") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysAttrNode>(m_size_str.c_str()).value());
  if (fk::memory::compare(name, "sector_size") == 0)
    return fk::RefPtr<Node>(fk::make_ref<SysAttrNode>(m_sector_size_str.c_str()).value());
  return fk::core::Error::NotFound;
}
