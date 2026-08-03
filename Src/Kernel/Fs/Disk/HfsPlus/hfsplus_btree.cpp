#include <Kernel/Fs/Disk/HfsPlus/hfsplus_btree.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_unicode.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Generic/byte_order.h>

namespace fkernel {

// ────────────────────────────────────────────────────────────────────────────
// BTreeNode helpers
// ────────────────────────────────────────────────────────────────────────────

uint16_t BTreeNode::record_offset(uint16_t index) const {
    if (data.is_empty()) return 0;
    size_t node_size = data.size();
    // Offsets are stored at the END of the node, growing backwards.
    // Offset for record[i] is at node[node_size - 2*(i+1)]
    size_t off_pos = node_size - 2 * (size_t)(index + 1);
    if (off_pos + 2 > node_size) return 0;
    uint16_t raw;
    fk::memory::copy(&raw, data.begin() + off_pos, 2);
    return fk::algorithms::swap16(raw);
}

const uint8_t* BTreeNode::record_ptr(uint16_t index) const {
    uint16_t off = record_offset(index);
    if (off == 0 || off >= data.size()) return nullptr;
    return data.begin() + off;
}

// ────────────────────────────────────────────────────────────────────────────
// BTreeFile
// ────────────────────────────────────────────────────────────────────────────

fk::core::Result<void, fk::core::Error> BTreeFile::open(
    fk::RefPtr<StorageDevice> device,
    const HFSPlusForkData& fork,
    uint32_t block_size,
    uint32_t first_sector)
{
    m_device       = device;
    m_fork         = fork;
    m_block_size   = block_size;
    m_first_sector = first_sector;
    m_node_size    = 0;

    // Read the B-tree header node (node 0).  We must first compute
    // which physical sector holds block 0 of this fork.
    uint64_t fork_logical_size = fk::algorithms::swap64(fork.logicalSize);
    if (fork_logical_size == 0) return fk::core::Error::InvalidData;

    // Assume the first extent contains node 0 at its startBlock.
    uint32_t start_block = fk::algorithms::swap32(fork.extents[0].startBlock);
    // Sector = first_sector + start_block * (block_size / sector_size)
    uint32_t sector_size   = device->sector_size().value();
    uint32_t sects_per_blk = block_size / sector_size;
    uint64_t node0_sector  = (uint64_t)first_sector + (uint64_t)start_block * sects_per_blk;

    // Read 512-byte header to get nodeSize before we know the full node
    fk::containers::Vector<uint8_t> hdr_buf;
    hdr_buf.resize(sector_size);
    if (device->read_sectors(node0_sector, 1, hdr_buf.begin()).is_error())
        return fk::core::Error::IOError;

    // The BTHeaderRec is at offset 14 (sizeof BTNodeDescriptor) in node 0
    fk::memory::copy(&m_header,
                     hdr_buf.begin() + sizeof(BTNodeDescriptor),
                     sizeof(BTHeaderRec));

    m_node_size = fk::algorithms::swap16(m_header.nodeSize);
    if (m_node_size < 512 || m_node_size > 32768)
        return fk::core::Error::InvalidData;

    // Re-read the full header node now that we know its size
    uint32_t sects_for_node = m_node_size / sector_size;
    fk::containers::Vector<uint8_t> node_buf;
    node_buf.resize(m_node_size);
    if (device->read_sectors(node0_sector, sects_for_node, node_buf.begin()).is_error())
        return fk::core::Error::IOError;

    fk::memory::copy(&m_header,
                     node_buf.begin() + sizeof(BTNodeDescriptor),
                     sizeof(BTHeaderRec));

    // Determine case-sensitivity from keyCompareType
    m_case_sensitive = (m_header.keyCompareType == kHFSBinaryCompare);
    return {};
}

fk::core::Result<uint64_t, fk::core::Error> BTreeFile::logical_to_physical(uint32_t logical_block) const {
    uint32_t remaining = logical_block;
    // Walk the 8 inline extents
    for (int i = 0; i < kHFSPlusExtentDensity; ++i) {
        uint32_t start = fk::algorithms::swap32(m_fork.extents[i].startBlock);
        uint32_t count = fk::algorithms::swap32(m_fork.extents[i].blockCount);
        if (count == 0) break;
        if (remaining < count) {
            uint32_t sector_size   = m_device->sector_size().value();
            uint32_t sects_per_blk = m_block_size / sector_size;
            return (uint64_t)m_first_sector + (uint64_t)(start + remaining) * sects_per_blk;
        }
        remaining -= count;
    }
    // Overflow — caller needs to use the extents B-tree
    return fk::core::Error::InvalidParameter;
}

fk::core::Result<BTreeNode, fk::core::Error> BTreeFile::read_node(uint32_t node_num) const {
    uint32_t block_index = (uint32_t)((uint64_t)node_num * m_node_size / m_block_size);
    uint32_t block_off   = (uint32_t)((uint64_t)node_num * m_node_size % m_block_size);

    auto phys_res = logical_to_physical(block_index);
    if (phys_res.is_error()) return phys_res.error();

    uint32_t sector_size   = m_device->sector_size().value();
    uint32_t sects_per_blk = m_block_size / sector_size;
    uint32_t node_sects    = m_node_size / sector_size;

    uint64_t start_sector = phys_res.value() + (uint64_t)(block_off / sector_size);

    BTreeNode node;
    node.data.resize(m_node_size);
    node.node_number = node_num;

    if (m_device->read_sectors(start_sector, node_sects, node.data.begin()).is_error())
        return fk::core::Error::IOError;

    (void)sects_per_blk;
    return node;
}

// ────────────────────────────────────────────────────────────────────────────
// Catalog key comparison
// ────────────────────────────────────────────────────────────────────────────

static int compare_catalog_keys(const HFSPlusCatalogKey* a,
                                const HFSPlusCatalogKey* b,
                                bool case_sensitive)
{
    uint32_t pa = fk::algorithms::swap32(a->parentID);
    uint32_t pb = fk::algorithms::swap32(b->parentID);
    if (pa != pb) return (pa < pb) ? -1 : 1;
    if (case_sensitive)
        return hfsplus_unicode_cmp_cs(a->nodeName, b->nodeName);
    return hfsplus_unicode_cmp_ci(a->nodeName, b->nodeName);
}

// ────────────────────────────────────────────────────────────────────────────
// Catalog B-tree search
// ────────────────────────────────────────────────────────────────────────────

fk::core::Result<CatalogRecord, fk::core::Error> btree_catalog_lookup(
    const BTreeFile& cat,
    uint32_t parent_id,
    const HFSUniStr255& name)
{
    HFSPlusCatalogKey search_key;
    fk::memory::set(&search_key, 0, sizeof(search_key));
    search_key.parentID = fk::algorithms::swap32(parent_id);
    fk::memory::copy(&search_key.nodeName, &name, sizeof(HFSUniStr255));
    uint16_t key_len = (uint16_t)(sizeof(uint32_t) + sizeof(uint16_t) +
                                  fk::algorithms::swap16(name.length) * sizeof(uint16_t));
    search_key.keyLength = fk::algorithms::swap16(key_len);

    uint32_t node_num = fk::algorithms::swap32(cat.header().rootNode);
    if (node_num == 0) return fk::core::Error::NotFound;

    for (;;) {
        auto node_res = cat.read_node(node_num);
        if (node_res.is_error()) return node_res.error();
        const BTreeNode& node = node_res.value();
        const BTNodeDescriptor* desc = node.descriptor();
        int8_t kind = desc->kind;
        uint16_t nr = fk::algorithms::swap16(desc->numRecords);

        if (kind == kBTLeafNode) {
            // Linear scan through leaf records
            for (uint16_t i = 0; i < nr; ++i) {
                const uint8_t* rec = node.record_ptr(i);
                if (!rec) continue;
                const auto* key = reinterpret_cast<const HFSPlusCatalogKey*>(rec);
                int cmp = compare_catalog_keys(&search_key, key, cat.is_case_sensitive());
                if (cmp == 0) {
                    // Skip past the key to the data record
                    uint16_t kl = fk::algorithms::swap16(key->keyLength);
                    // Key length field is 2 bytes; total key bytes = kl + 2, rounded to even
                    uint16_t key_total = (uint16_t)(kl + 2);
                    if (key_total & 1) key_total++;
                    const uint8_t* data_rec = rec + key_total;

                    CatalogRecord result;
                    int16_t rec_type;
                    fk::memory::copy(&rec_type, data_rec, 2);
                    rec_type = (int16_t)fk::algorithms::swap16((uint16_t)rec_type);
                    result.type = rec_type;
                    if (rec_type == kHFSPlusFolderRecord)
                        fk::memory::copy(&result.folder, data_rec, sizeof(HFSPlusCatalogFolder));
                    else if (rec_type == kHFSPlusFileRecord)
                        fk::memory::copy(&result.file, data_rec, sizeof(HFSPlusCatalogFile));
                    else if (rec_type == kHFSPlusFolderThreadRecord || rec_type == kHFSPlusFileThreadRecord)
                        fk::memory::copy(&result.thread, data_rec, sizeof(HFSPlusCatalogThread));
                    return result;
                }
                if (cmp < 0) return fk::core::Error::NotFound;
            }
            return fk::core::Error::NotFound;
        }

        if (kind != kBTIndexNode) return fk::core::Error::InvalidData;

        // Index node: binary search for the greatest key ≤ search_key
        uint32_t next_node = 0;
        bool found = false;
        for (uint16_t i = 0; i < nr; ++i) {
            const uint8_t* rec = node.record_ptr(i);
            if (!rec) break;
            const auto* key = reinterpret_cast<const HFSPlusCatalogKey*>(rec);
            int cmp = compare_catalog_keys(&search_key, key, cat.is_case_sensitive());
            if (cmp < 0) break;
            // Pointer is immediately after key (rounded to even)
            uint16_t kl = fk::algorithms::swap16(key->keyLength);
            uint16_t key_total = (uint16_t)(kl + 2);
            if (key_total & 1) key_total++;
            uint32_t child;
            fk::memory::copy(&child, rec + key_total, 4);
            next_node = fk::algorithms::swap32(child);
            found = true;
        }
        if (!found) return fk::core::Error::NotFound;
        node_num = next_node;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Catalog B-tree list (enumerate children of parentID)
// ────────────────────────────────────────────────────────────────────────────

fk::core::Result<void, fk::core::Error> btree_catalog_list(
    const BTreeFile& cat,
    uint32_t parent_id,
    CatalogCallback cb,
    void* ctx)
{
    // Build a search key with an empty name to find the first record in the parent
    HFSPlusCatalogKey search_key;
    fk::memory::set(&search_key, 0, sizeof(search_key));
    search_key.parentID  = fk::algorithms::swap32(parent_id);
    search_key.keyLength = fk::algorithms::swap16(sizeof(uint32_t) + sizeof(uint16_t));

    // Navigate to the first leaf node that could contain our parentID
    uint32_t node_num = fk::algorithms::swap32(cat.header().rootNode);
    if (node_num == 0) return {};

    // Descend the index nodes to find the leaf
    for (;;) {
        auto node_res = cat.read_node(node_num);
        if (node_res.is_error()) return node_res.error();
        const BTreeNode& node = node_res.value();
        const BTNodeDescriptor* desc = node.descriptor();
        if (desc->kind == kBTLeafNode) {
            // Walk forward through leaves, calling cb for matching records
            uint32_t cur = node_num;
            while (cur != 0) {
                auto cur_res = cat.read_node(cur);
                if (cur_res.is_error()) break;
                const BTreeNode& leaf = cur_res.value();
                const BTNodeDescriptor* ld = leaf.descriptor();
                uint16_t nr = fk::algorithms::swap16(ld->numRecords);

                for (uint16_t i = 0; i < nr; ++i) {
                    const uint8_t* rec = leaf.record_ptr(i);
                    if (!rec) continue;
                    const auto* key = reinterpret_cast<const HFSPlusCatalogKey*>(rec);
                    uint32_t rec_parent = fk::algorithms::swap32(key->parentID);
                    if (rec_parent < parent_id) continue;
                    if (rec_parent > parent_id) goto done;

                    uint16_t kl       = fk::algorithms::swap16(key->keyLength);
                    uint16_t key_total = (uint16_t)(kl + 2);
                    if (key_total & 1) key_total++;
                    const uint8_t* data_rec = rec + key_total;

                    int16_t rec_type;
                    fk::memory::copy(&rec_type, data_rec, 2);
                    rec_type = (int16_t)fk::algorithms::swap16((uint16_t)rec_type);

                    // Skip thread records; only emit file + folder
                    if (rec_type != kHFSPlusFileRecord && rec_type != kHFSPlusFolderRecord)
                        continue;

                    char name_utf8[768];
                    hfsplus_unicode_to_utf8(key->nodeName, name_utf8, sizeof(name_utf8));

                    CatalogRecord cr;
                    cr.type = rec_type;
                    if (rec_type == kHFSPlusFolderRecord)
                        fk::memory::copy(&cr.folder, data_rec, sizeof(HFSPlusCatalogFolder));
                    else
                        fk::memory::copy(&cr.file, data_rec, sizeof(HFSPlusCatalogFile));
                    cb(ctx, name_utf8, cr);
                }
                cur = fk::algorithms::swap32(ld->fLink);
            }
            goto done;
        }

        // Index node: pick the correct child
        uint32_t next = 0;
        uint16_t nr   = fk::algorithms::swap16(desc->numRecords);
        for (uint16_t i = 0; i < nr; ++i) {
            const uint8_t* rec = node.record_ptr(i);
            if (!rec) break;
            const auto* key = reinterpret_cast<const HFSPlusCatalogKey*>(rec);
            int cmp = (int)fk::algorithms::swap32(key->parentID) - (int)parent_id;
            if (cmp > 0) break;
            uint16_t kl = fk::algorithms::swap16(key->keyLength);
            uint16_t key_total = (uint16_t)(kl + 2);
            if (key_total & 1) key_total++;
            uint32_t child;
            fk::memory::copy(&child, rec + key_total, 4);
            next = fk::algorithms::swap32(child);
        }
        if (next == 0) goto done;
        node_num = next;
    }
done:
    return {};
}

// ────────────────────────────────────────────────────────────────────────────
// Extents overflow B-tree lookup
// ────────────────────────────────────────────────────────────────────────────

fk::core::Result<fk::containers::Vector<HFSPlusExtentDescriptor>, fk::core::Error>
btree_extents_lookup(const BTreeFile& ext, uint32_t file_id,
                     uint8_t fork_type, uint32_t start_block)
{
    HFSPlusExtentKey search_key;
    fk::memory::set(&search_key, 0, sizeof(search_key));
    search_key.keyLength   = fk::algorithms::swap16(sizeof(HFSPlusExtentKey) - 2);
    search_key.forkType    = fork_type;
    search_key.fileID      = fk::algorithms::swap32(file_id);
    search_key.startBlock  = fk::algorithms::swap32(start_block);

    uint32_t node_num = fk::algorithms::swap32(ext.header().rootNode);
    if (node_num == 0) return fk::core::Error::NotFound;

    for (;;) {
        auto node_res = ext.read_node(node_num);
        if (node_res.is_error()) return node_res.error();
        const BTreeNode& node = node_res.value();
        const BTNodeDescriptor* desc = node.descriptor();
        uint16_t nr = fk::algorithms::swap16(desc->numRecords);

        if (desc->kind == kBTLeafNode) {
            for (uint16_t i = 0; i < nr; ++i) {
                const uint8_t* rec = node.record_ptr(i);
                if (!rec) continue;
                const auto* key = reinterpret_cast<const HFSPlusExtentKey*>(rec);
                if (fk::algorithms::swap32(key->fileID) != file_id) continue;
                if (key->forkType != fork_type) continue;
                if (fk::algorithms::swap32(key->startBlock) != start_block) continue;

                uint16_t kl = fk::algorithms::swap16(key->keyLength);
                uint16_t key_total = (uint16_t)(kl + 2);
                if (key_total & 1) key_total++;
                const uint8_t* data = rec + key_total;

                fk::containers::Vector<HFSPlusExtentDescriptor> result;
                for (int j = 0; j < kHFSPlusExtentDensity; ++j) {
                    HFSPlusExtentDescriptor ed;
                    fk::memory::copy(&ed, data + j * sizeof(HFSPlusExtentDescriptor), sizeof(ed));
                    if (fk::algorithms::swap32(ed.blockCount) == 0) break;
                    result.push_back(ed);
                }
                return result;
            }
            return fk::core::Error::NotFound;
        }

        if (desc->kind != kBTIndexNode) return fk::core::Error::InvalidData;

        uint32_t next = 0;
        bool found = false;
        for (uint16_t i = 0; i < nr; ++i) {
            const uint8_t* rec = node.record_ptr(i);
            if (!rec) break;
            const auto* key = reinterpret_cast<const HFSPlusExtentKey*>(rec);
            uint32_t k_fid = fk::algorithms::swap32(key->fileID);
            uint32_t k_sb  = fk::algorithms::swap32(key->startBlock);
            bool beyond = (k_fid > file_id) ||
                          (k_fid == file_id && key->forkType > fork_type) ||
                          (k_fid == file_id && key->forkType == fork_type && k_sb > start_block);
            if (beyond) break;
            uint16_t kl = fk::algorithms::swap16(key->keyLength);
            uint16_t key_total = (uint16_t)(kl + 2);
            if (key_total & 1) key_total++;
            uint32_t child;
            fk::memory::copy(&child, rec + key_total, 4);
            next = fk::algorithms::swap32(child);
            found = true;
        }
        if (!found) return fk::core::Error::NotFound;
        node_num = next;
    }
}

} // namespace fkernel
