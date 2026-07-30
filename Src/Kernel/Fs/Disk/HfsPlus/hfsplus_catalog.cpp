#include <Kernel/Fs/Disk/HfsPlus/hfsplus_catalog.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

HFSPlusCatalogKey make_catalog_key(uint32_t parent_id, const HFSUniStr255& name) {
    HFSPlusCatalogKey key;
    fk::memory::set(&key, 0, sizeof(key));
    key.parentID = hfs_be32(parent_id);
    fk::memory::copy(&key.nodeName, &name, sizeof(HFSUniStr255));
    uint16_t key_len = (uint16_t)(sizeof(uint32_t) + sizeof(uint16_t) +
                                  hfs_be16(name.length) * sizeof(uint16_t));
    key.keyLength = hfs_be16(key_len);
    return key;
}

} // namespace fkernel
