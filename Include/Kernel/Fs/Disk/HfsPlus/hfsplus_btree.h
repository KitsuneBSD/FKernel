#pragma once

#include <Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fkernel {

// Opaque buffer for a single B-tree node (nodeSize bytes)
struct BTreeNode {
    fk::containers::Vector<uint8_t> data;
    uint32_t node_number{0};

    const BTNodeDescriptor* descriptor() const {
        return reinterpret_cast<const BTNodeDescriptor*>(data.begin());
    }
    BTNodeDescriptor* descriptor() {
        return reinterpret_cast<BTNodeDescriptor*>(data.begin());
    }

    // Offset of the N-th record in the node (from the node-offset table at the end)
    uint16_t record_offset(uint16_t index) const;

    // Pointer to the N-th record
    const uint8_t* record_ptr(uint16_t index) const;
};

// Thin wrapper around a special-file fork for B-tree I/O.
// Translates logical block numbers → physical sectors via the fork's extents.
class BTreeFile {
public:
    BTreeFile() = default;

    fk::core::Result<void, fk::core::Error> open(
        fk::RefPtr<StorageDevice> device,
        const HFSPlusForkData& fork,
        uint32_t block_size,
        uint32_t first_sector);

    // Read one B-tree node by number.  Allocates BTreeNode::data.
    fk::core::Result<BTreeNode, fk::core::Error> read_node(uint32_t node_num) const;

    const BTHeaderRec& header() const { return m_header; }
    uint16_t node_size()        const { return m_node_size; }
    bool     is_case_sensitive()const { return m_case_sensitive; }

private:
    fk::core::Result<uint64_t, fk::core::Error> logical_to_physical(uint32_t logical_block) const;

    fk::RefPtr<StorageDevice> m_device;
    HFSPlusForkData           m_fork{};
    BTHeaderRec               m_header{};
    uint32_t                  m_block_size{0};
    uint32_t                  m_first_sector{0};
    uint16_t                  m_node_size{0};
    bool                      m_case_sensitive{false};
};

// Result of a catalog search — union of folder/file record
struct CatalogRecord {
    int16_t type{0};
    union {
        HFSPlusCatalogFolder folder;
        HFSPlusCatalogFile   file;
        HFSPlusCatalogThread thread;
    };
};

// Search the catalog B-tree for (parentID, name).  Returns the matching record.
fk::core::Result<CatalogRecord, fk::core::Error> btree_catalog_lookup(
    const BTreeFile& cat,
    uint32_t parent_id,
    const HFSUniStr255& name);

// Walk the catalog B-tree and enumerate all direct children of parentID.
// Calls cb(name_utf8, record) for each entry (files + folders).
using CatalogCallback = void(*)(void* ctx, const char* name, const CatalogRecord& rec);
fk::core::Result<void, fk::core::Error> btree_catalog_list(
    const BTreeFile& cat,
    uint32_t parent_id,
    CatalogCallback cb,
    void* ctx);

// Look up overflow extents for (fileID, forkType, startBlock).
// Returns a vector of HFSPlusExtentDescriptor records that cover the range.
fk::core::Result<fk::containers::Vector<HFSPlusExtentDescriptor>, fk::core::Error>
btree_extents_lookup(const BTreeFile& ext, uint32_t file_id,
                     uint8_t fork_type, uint32_t start_block);

} // namespace fkernel
