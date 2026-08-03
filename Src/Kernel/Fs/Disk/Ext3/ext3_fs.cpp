#include <Kernel/Fs/Disk/Ext3/ext3_fs.h>
#include <Kernel/Fs/Disk/Ext3/ext3_super.h>
#include <Kernel/Fs/Disk/Ext2/ext2_super.h>
#include <LibFK/Algorithms/Generic/byte_order.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

namespace fkernel {

using fk::core::Error;
using fk::core::Result;

// ---- Journal block access helpers --------------------------------------------

// Read block number from a 32-bit indirect block buffer.
static uint32_t read_indir_entry(const uint8_t* buf, uint32_t index) {
    uint32_t val = 0;
    fk::memory::copy(&val, buf + index * 4, 4);
    return val;
}

// Get the FS block number for logical block N of the journal inode.
// Supports direct (0-11) and single-indirect (12..12+ptrs_per_block).
static uint32_t journal_inode_block(const Ext2Inode& inode, uint32_t logical,
                                     uint32_t block_size,
                                     fk::RefPtr<StorageDevice> device) {
    if (logical < 12) return inode.i_block[logical];

    uint32_t ptrs = block_size / 4;
    uint32_t si_idx = logical - 12;
    if (si_idx < ptrs) {
        uint8_t* buf = static_cast<uint8_t*>(kmalloc(block_size));
        if (!buf) return 0;
        if (device->read(static_cast<uint64_t>(inode.i_block[12]) * block_size,
                         block_size, buf).is_error()) {
            kfree(buf);
            return 0;
        }
        uint32_t result = read_indir_entry(buf, si_idx);
        kfree(buf);
        return result;
    }
    return 0; // double/triple indirect not needed for typical journal sizes
}

// ---- Pending replay block list -----------------------------------------------

static constexpr uint32_t MAX_PENDING = 512;

struct PendingBlock {
    uint32_t fs_block;
    uint32_t journal_block; // logical index within journal inode
};

// ---- Journal recovery --------------------------------------------------------

static bool replay_journal(fk::RefPtr<StorageDevice> device,
                            const Ext2Superblock& /*sb*/,
                            const Ext2Inode& journal_inode,
                            uint32_t block_size) {
    // Read journal superblock (journal inode's logical block 0)
    uint32_t js_fs_block = journal_inode.i_block[0];
    if (js_fs_block == 0) return false;

    uint8_t* jsbuf = static_cast<uint8_t*>(kmalloc(block_size));
    if (!jsbuf) return false;

    if (device->read(static_cast<uint64_t>(js_fs_block) * block_size,
                     block_size, jsbuf).is_error()) {
        kfree(jsbuf);
        return false;
    }

    const JbdSuperblock* jsb = reinterpret_cast<const JbdSuperblock*>(jsbuf);
    if (fk::algorithms::swap32(jsb->s_header.h_magic) != JBD_MAGIC_NUMBER) {
        fk::algorithms::kwarn("EXT3", "Journal has bad magic — skipping recovery");
        kfree(jsbuf);
        return true; // assume clean
    }

    uint32_t j_start    = fk::algorithms::swap32(jsb->s_start);
    uint32_t j_sequence = fk::algorithms::swap32(jsb->s_sequence);
    uint32_t j_maxlen   = fk::algorithms::swap32(jsb->s_maxlen);
    uint32_t j_first    = fk::algorithms::swap32(jsb->s_first);
    kfree(jsbuf);

    if (j_start == 0) {
        fk::algorithms::kdebug("EXT3", "Journal is clean — no recovery needed");
        return true;
    }

    fk::algorithms::klog("EXT3", "Replaying journal: start=%u seq=%u maxlen=%u",
                         j_start, j_sequence, j_maxlen);

    uint8_t* blk_buf  = static_cast<uint8_t*>(kmalloc(block_size));
    uint8_t* data_buf = static_cast<uint8_t*>(kmalloc(block_size));
    if (!blk_buf || !data_buf) {
        if (blk_buf)  kfree(blk_buf);
        if (data_buf) kfree(data_buf);
        return false;
    }

    PendingBlock pending[MAX_PENDING];
    uint32_t pending_count = 0;
    uint32_t curr = j_start;
    uint32_t curr_seq = j_sequence;
    bool ok = true;

    for (uint32_t iter = 0; iter < j_maxlen * 2 && ok; iter++) {
        uint32_t fs_blk = journal_inode_block(journal_inode, curr, block_size, device);
        if (fs_blk == 0) { ok = false; break; }

        if (device->read(static_cast<uint64_t>(fs_blk) * block_size,
                         block_size, blk_buf).is_error()) {
            ok = false;
            break;
        }

        const JbdHeader* hdr = reinterpret_cast<const JbdHeader*>(blk_buf);
        if (fk::algorithms::swap32(hdr->h_magic) != JBD_MAGIC_NUMBER) break;
        if (fk::algorithms::swap32(hdr->h_sequence) != curr_seq) break;

        uint32_t btype = fk::algorithms::swap32(hdr->h_blocktype);

        if (btype == JBD_DESCRIPTOR_BLOCK) {
            pending_count = 0;
            uint32_t tag_off = sizeof(JbdHeader);

            while (tag_off + sizeof(JbdBlockTag) <= block_size) {
                const JbdBlockTag* tag =
                    reinterpret_cast<const JbdBlockTag*>(blk_buf + tag_off);
                uint32_t fs_block  = fk::algorithms::swap32(tag->t_blocknr);
                uint16_t flags     = fk::algorithms::swap16(tag->t_flags);
                tag_off += sizeof(JbdBlockTag);
                if (!(flags & JBD_FLAG_SAME_UUID)) tag_off += 16;

                // Next block in stream is the data block for this tag
                curr = (curr - j_first + 1) % (j_maxlen - j_first) + j_first;

                if (pending_count < MAX_PENDING) {
                    pending[pending_count++] = {fs_block, curr};
                }

                if (flags & JBD_FLAG_LAST_TAG) break;

                // Advance past the data block
                curr = (curr - j_first + 1) % (j_maxlen - j_first) + j_first;
            }
            continue; // curr was updated inside the loop
        }

        if (btype == JBD_COMMIT_BLOCK) {
            // Apply all pending blocks
            for (uint32_t i = 0; i < pending_count && ok; i++) {
                uint32_t jfs_blk = journal_inode_block(journal_inode,
                                                        pending[i].journal_block,
                                                        block_size, device);
                if (jfs_blk == 0) continue;
                if (device->read(static_cast<uint64_t>(jfs_blk) * block_size,
                                 block_size, data_buf).is_error()) continue;
                device->write(static_cast<uint64_t>(pending[i].fs_block) * block_size,
                              block_size, data_buf);
            }
            pending_count = 0;
            curr_seq++;
        }

        if (btype == JBD_REVOKE_BLOCK) {
            // Revoke: invalidate pending blocks listed in this block
            // (simple implementation: just clear all pending for revoked blocks)
            uint32_t count_off = sizeof(JbdHeader);
            uint32_t revoke_count = 0;
            fk::memory::copy(&revoke_count, blk_buf + count_off, 4);
            revoke_count = fk::algorithms::swap32(revoke_count);
            count_off += 4;
            for (uint32_t r = 0; r < revoke_count && count_off + 4 <= block_size; r++, count_off += 4) {
                uint32_t rev_blk = 0;
                fk::memory::copy(&rev_blk, blk_buf + count_off, 4);
                rev_blk = fk::algorithms::swap32(rev_blk);
                // Mark matching pending blocks as invalid
                for (uint32_t p = 0; p < pending_count; p++) {
                    if (pending[p].fs_block == rev_blk) pending[p].fs_block = 0;
                }
            }
        }

        curr = (curr - j_first + 1) % (j_maxlen - j_first) + j_first;
    }

    kfree(blk_buf);
    kfree(data_buf);

    // Clear journal start in journal superblock to mark clean
    uint8_t* jsbuf2 = static_cast<uint8_t*>(kmalloc(block_size));
    if (jsbuf2) {
        if (device->read(static_cast<uint64_t>(js_fs_block) * block_size,
                         block_size, jsbuf2).is_ok()) {
            JbdSuperblock* jsb2 = reinterpret_cast<JbdSuperblock*>(jsbuf2);
            jsb2->s_start = 0;
            device->write(static_cast<uint64_t>(js_fs_block) * block_size,
                          block_size, jsbuf2);
        }
        kfree(jsbuf2);
    }

    fk::algorithms::klog("EXT3", "Journal recovery complete (seq=%u)", curr_seq);
    return ok;
}

// ---- create() factory --------------------------------------------------------

Result<fk::RefPtr<Ext3FileSystem>, Error>
Ext3FileSystem::create(fk::RefPtr<StorageDevice> device) {
    Ext2Superblock sb;
    if (device->read(1024, sizeof(sb), reinterpret_cast<uint8_t*>(&sb)).is_error())
        return Error::IOError;

    if (sb.s_magic != EXT2_SUPER_MAGIC) return Error::InvalidData;

    // Must have the journal compat feature
    if (!(sb.s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL))
        return Error::InvalidData;

    uint32_t block_size = 1024u << sb.s_log_block_size;

    // Handle INCOMPAT_RECOVER: replay journal before creating Ext2FileSystem
    if (sb.s_rev_level >= 1 && (sb.s_feature_incompat & EXT2_INCOMPAT_RECOVER)) {
        fk::algorithms::klog("EXT3", "Replaying dirty journal for ext3 filesystem");

        // Read inode 8 (journal inode)
        Ext2BlockGroupDesc bg0;
        uint32_t bgdt_byte = block_size * (sb.s_first_data_block + 1);
        if (device->read(bgdt_byte, sizeof(bg0),
                         reinterpret_cast<uint8_t*>(&bg0)).is_error())
            return Error::IOError;

        uint16_t inode_size = (sb.s_rev_level >= 1 && sb.s_inode_size >= 128)
                              ? sb.s_inode_size : 128;
        uint32_t local_idx = (EXT3_JOURNAL_INO - 1) % sb.s_inodes_per_group;
        uint64_t inode_off = static_cast<uint64_t>(bg0.bg_inode_table) * block_size
                           + static_cast<uint64_t>(local_idx) * inode_size;

        Ext2Inode journal_inode;
        fk::memory::set(&journal_inode, 0, sizeof(journal_inode));
        if (device->read(inode_off, sizeof(journal_inode),
                         reinterpret_cast<uint8_t*>(&journal_inode)).is_error())
            return Error::IOError;

        if (!replay_journal(device, sb, journal_inode, block_size))
            fk::algorithms::kwarn("EXT3", "Journal replay had errors — continuing anyway");

        // Clear INCOMPAT_RECOVER in on-disk superblock
        sb.s_feature_incompat &= ~EXT2_INCOMPAT_RECOVER;
        device->write(1024 + offsetof(Ext2Superblock, s_feature_incompat),
                      sizeof(sb.s_feature_incompat),
                      reinterpret_cast<const uint8_t*>(&sb.s_feature_incompat));
    }

    // Now create Ext2FileSystem (journal recovery cleared INCOMPAT_RECOVER)
    auto ext2_res = Ext2FileSystem::create(device);
    if (ext2_res.is_error()) {
        fk::algorithms::kwarn("EXT3", "Failed to create ext2 layer: error=%d",
                               (int)ext2_res.error());
        return ext2_res.error();
    }

    auto fs = fk::adopt_ref(new Ext3FileSystem(ext2_res.value()));
    if (!fs) return Error::OutOfMemory;

    fk::algorithms::klog("EXT3", "ext3 mounted (journal recovered + delegating to ext2)");
    return fs;
}

} // namespace fkernel
