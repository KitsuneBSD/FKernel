#include <Kernel/Fs/Disk/Fat32/fat_32_node.h>
#include <Kernel/Fs/Disk/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Disk/Fat32/directory_entry.h>
#include <LibFK/Algorithms/fat_name.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

static void name_to_83(const char* src, char* name_out, char* ext_out) {
    fk::memory::set(name_out, ' ', 8);
    fk::memory::set(ext_out, ' ', 3);
    int ni = 0, ei = 0;
    bool in_ext = false;
    for (int i = 0; src[i] && ni < 8; ++i) {
        char c = src[i];
        if (c == '.') { in_ext = true; continue; }
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (in_ext) {
            if (ei < 3) ext_out[ei++] = c;
        } else {
            name_out[ni++] = c;
        }
    }
}

Fat32Node::Fat32Node(fk::RefPtr<Fat32FileSystem> fs, uint32_t cluster, size_t size, bool is_dir,
                     uint32_t dir_sector, uint8_t dir_entry_idx)
    : m_fs(fs), m_first_cluster(cluster), m_size(size), m_is_dir(is_dir),
      m_dir_sector(dir_sector), m_dir_entry_idx(dir_entry_idx) {}

fk::core::Result<size_t, fk::core::Error>
Fat32Node::read(uint64_t offset, size_t size, uint8_t *buffer) {
    if (offset >= m_size) return 0;
    if (offset + size > m_size) size = m_size - offset;
    return m_fs->read_from_cluster_chain(m_first_cluster, offset, size, buffer);
}

fk::core::Result<size_t, fk::core::Error>
Fat32Node::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    if (m_is_dir) return fk::core::Error::IsDirectory;
    auto res = m_fs->write_to_cluster_chain(m_first_cluster, offset, size, buffer, (size_t&)m_size);
    if (!res.is_error() && m_dir_sector != 0)
        (void)m_fs->update_dir_entry_size(m_dir_sector, m_dir_entry_idx, (uint32_t)m_size);
    return res;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> Fat32Node::lookup(const char* name) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    return m_fs->find_in_directory(m_first_cluster, name);
}

fk::core::Result<void, fk::core::Error> Fat32Node::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    return m_fs->list_directory_from(m_first_cluster, entries);
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> Fat32Node::create_child(const char* name, int) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;

    uint32_t sec; uint8_t idx;
    auto res = m_fs->find_free_dir_entry(m_first_cluster, sec, idx);
    if (res.is_error()) return res.error();

    auto child = fk::adopt_ref(new Fat32Node(m_fs, 0, 0, false, sec, idx));
    if (!child) return fk::core::Error::OutOfMemory;

    Fat32DirectoryEntry entry;
    fk::memory::set(&entry, 0, sizeof(entry));
    name_to_83(name, entry.name, entry.ext);
    entry.attr = 0x20;
    entry.cluster_low = 0;
    entry.cluster_high = 0;
    entry.size = 0;

    auto write_res = m_fs->write_dir_entry_raw(sec, idx, entry);
    if (write_res.is_error()) return write_res.error();

    return fk::RefPtr<Node>(child.ptr());
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> Fat32Node::mkdir(const char* name, int) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;

    auto cluster_res = m_fs->allocate_cluster(0);
    if (cluster_res.is_error()) return cluster_res.error();
    uint32_t dir_cluster = cluster_res.value();

    uint32_t parent_sec; uint8_t parent_idx;
    auto entry_res = m_fs->find_free_dir_entry(m_first_cluster, parent_sec, parent_idx);
    if (entry_res.is_error()) { m_fs->free_cluster_chain(dir_cluster); return entry_res.error(); }

    uint32_t new_dir_sector = m_fs->cluster_to_sector(dir_cluster);

    Fat32DirectoryEntry dot;
    fk::memory::set(&dot, 0, sizeof(dot));
    fk::memory::copy(dot.name, ".          ", 11);
    dot.attr = 0x10;
    dot.cluster_low = dir_cluster & 0xFFFF;
    dot.cluster_high = (dir_cluster >> 16) & 0xFFFF;
    (void)m_fs->write_dir_entry_raw(new_dir_sector, 0, dot);

    Fat32DirectoryEntry dotdot;
    fk::memory::set(&dotdot, 0, sizeof(dotdot));
    fk::memory::copy(dotdot.name, "..         ", 11);
    dotdot.attr = 0x10;
    dotdot.cluster_low = m_first_cluster & 0xFFFF;
    dotdot.cluster_high = (m_first_cluster >> 16) & 0xFFFF;
    (void)m_fs->write_dir_entry_raw(new_dir_sector, 1, dotdot);

    Fat32DirectoryEntry parent_entry;
    fk::memory::set(&parent_entry, 0, sizeof(parent_entry));
    name_to_83(name, parent_entry.name, parent_entry.ext);
    parent_entry.attr = 0x10;
    parent_entry.cluster_low = dir_cluster & 0xFFFF;
    parent_entry.cluster_high = (dir_cluster >> 16) & 0xFFFF;

    auto write_res = m_fs->write_dir_entry_raw(parent_sec, parent_idx, parent_entry);
    if (write_res.is_error()) return write_res.error();

    auto node = fk::adopt_ref(new Fat32Node(m_fs, dir_cluster, 0, true, parent_sec, parent_idx));
    if (!node) return fk::core::Error::OutOfMemory;
    return fk::RefPtr<Node>(node.ptr());
}

fk::core::Result<void, fk::core::Error> Fat32Node::unlink(const char* name) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;

    auto node_res = m_fs->find_in_directory(m_first_cluster, name);
    if (node_res.is_error()) return node_res.error();

    auto node_ptr = node_res.value().get();
    auto* fat_node = static_cast<Fat32Node*>(node_ptr);
    if (fat_node->is_directory()) return fk::core::Error::IsDirectory;

    if (fat_node->m_first_cluster >= 2)
        m_fs->free_cluster_chain(fat_node->m_first_cluster);

    Fat32DirectoryEntry deleted;
    fk::memory::set(&deleted, 0, sizeof(deleted));
    deleted.name[0] = (char)0xE5;
    return m_fs->write_dir_entry_raw(fat_node->m_dir_sector, fat_node->m_dir_entry_idx, deleted);
}

fk::core::Result<void, fk::core::Error> Fat32Node::rmdir(const char* name) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;

    auto node_res = m_fs->find_in_directory(m_first_cluster, name);
    if (node_res.is_error()) return node_res.error();

    auto node_ptr = node_res.value().get();
    auto* fat_node = static_cast<Fat32Node*>(node_ptr);
    if (!fat_node->is_directory()) return fk::core::Error::NotADirectory;

    fk::containers::Vector<DirectoryEntry> entries;
    auto list_res = m_fs->list_directory_from(fat_node->m_first_cluster, entries);
    if (!list_res.is_error()) {
        for (auto& e : entries) {
            if (e.type == 0) continue;
            bool is_dot = e.name[0] == '.' && (e.name[1] == '\0' || (e.name[1] == '.' && e.name[2] == '\0'));
            if (!is_dot) return fk::core::Error::DirectoryNotEmpty;
        }
    }

    if (fat_node->m_first_cluster >= 2)
        m_fs->free_cluster_chain(fat_node->m_first_cluster);

    Fat32DirectoryEntry deleted;
    fk::memory::set(&deleted, 0, sizeof(deleted));
    deleted.name[0] = (char)0xE5;
    return m_fs->write_dir_entry_raw(fat_node->m_dir_sector, fat_node->m_dir_entry_idx, deleted);
}

fk::core::Result<void, fk::core::Error> Fat32Node::truncate(uint64_t new_size) {
    if (m_is_dir) return fk::core::Error::IsDirectory;
    if (new_size == m_size) return {};

    if (new_size == 0) {
        if (m_first_cluster >= 2) {
            m_fs->free_cluster_chain(m_first_cluster);
            m_first_cluster = 0;
        }
        m_size = 0;
        if (m_dir_sector != 0)
            return m_fs->update_dir_entry_size(m_dir_sector, m_dir_entry_idx, 0);
        return {};
    }

    uint32_t cluster_size = m_fs->m_sectors_per_cluster * 512;

    if (new_size < m_size) {
        uint64_t bytes_to_walk = new_size - 1;
        uint64_t clusters_to_skip = bytes_to_walk / cluster_size;

        uint32_t current = m_first_cluster;
        for (uint64_t i = 0; i < clusters_to_skip && current < 0x0FFFFFF8; ++i)
            current = m_fs->get_next_cluster(current);

        if (current < 0x0FFFFFF8) {
            uint32_t next = m_fs->get_next_cluster(current);
            if (next < 0x0FFFFFF8) {
                (void)m_fs->write_fat_entry(current, 0x0FFFFFFF);
                m_fs->free_cluster_chain(next);
            }
        }
    } else {
        uint32_t current = m_first_cluster;
        if (current < 2) {
            auto res = m_fs->allocate_cluster(0);
            if (res.is_error()) return res.error();
            m_first_cluster = res.value();
            current = m_first_cluster;
        }

        uint32_t last = current;
        uint32_t cluster_count = 1;
        while (true) {
            uint32_t next = m_fs->get_next_cluster(last);
            if (next >= 0x0FFFFFF8) break;
            last = next;
            cluster_count++;
        }

        uint64_t total_needed = (new_size + (uint64_t)cluster_size - 1) / cluster_size;
        while (cluster_count < (uint32_t)total_needed) {
            auto res = m_fs->allocate_cluster(last);
            if (res.is_error()) return res.error();
            last = res.value();
            cluster_count++;
        }
    }

    m_size = new_size;
    if (m_dir_sector != 0)
        return m_fs->update_dir_entry_size(m_dir_sector, m_dir_entry_idx, (uint32_t)m_size);
    return {};
}

}
