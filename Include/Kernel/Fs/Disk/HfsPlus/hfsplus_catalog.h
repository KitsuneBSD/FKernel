#pragma once

#include <Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_btree.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

// Re-export BTreeFile + catalog helpers already declared in hfsplus_btree.h.
// This header exists so HFSPlusNode can include catalog types without a full
// hfsplus_btree.h chain.  Clients include this header for catalog-only work.

namespace fkernel {

// Build the HFSPlusCatalogKey for (parentID, name).
HFSPlusCatalogKey make_catalog_key(uint32_t parent_id, const HFSUniStr255& name);

} // namespace fkernel
