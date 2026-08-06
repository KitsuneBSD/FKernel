#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Generic/byte_order.h>

#include <Kernel/Fs/Disk/HfsPlus/hfsplus_catalog.h>

namespace fkernel {

HFSPlusCatalogKey make_catalog_key(uint32_t parent_id, const HFSUniStr255& name) {
    HFSPlusCatalogKey key;
    fk::memory::set(&key, 0, sizeof(key));
    key.parentID = fk::algorithms::swap32(parent_id);
    fk::memory::copy(&key.nodeName, &name, sizeof(HFSUniStr255));
    uint16_t key_len = (uint16_t)(sizeof(uint32_t) + sizeof(uint16_t) +
                                  fk::algorithms::swap16(name.length) * sizeof(uint16_t));
    key.keyLength = fk::algorithms::swap16(key_len);
    return key;
}

} // namespace fkernel
