#include <Kernel/Fs/RamDisk/ram_disk.h>
#include <LibFK/Utilities/Memory.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>

namespace fkernel {

fk::core::Result<size_t, fk::core::Error> RamFileNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (offset >= m_size) return 0;
    size_t to_read = (offset + size > m_size) ? (m_size - offset) : size;
    fk::memory::copy(buffer, m_data + offset, to_read);
    return to_read;
}

fk::core::Result<fk::RefPtr<RamDiskNode>, fk::core::Error> RamDiskNode::create(uintptr_t start, uintptr_t end) {
    auto res = TRY(fk::make_ref<RamDiskNode>(start, end));
    res->parse_tar();
    return fk::core::Result<fk::RefPtr<RamDiskNode>>(res);
}

void RamDiskNode::parse_tar() {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(m_start);
    fk::algorithms::klog("RAMDISK", "Parsing TAR at %p - %p (size: %zu)", 
                         (void*)m_start, (void*)m_end, m_end - m_start);
    
    size_t files_loaded = 0;
    while (ptr < reinterpret_cast<uint8_t*>(m_end)) {
        auto* header = reinterpret_cast<fk::archive::TarHeader*>(ptr);
        if (header->filename[0] == '\0') {
            fk::algorithms::klog("RAMDISK", "End of TAR found (null filename)");
            break;
        }

        // Lenient magic check: some TARs don't have ustar magic
        if (header->magic[0] != '\0' && strncmp(header->magic, "ustar", 5) != 0) {
            fk::algorithms::kwarn("RAMDISK", "Suspicious TAR magic at %p: '%.6s'", ptr, header->magic);
        }

        char safe_filename[101];
        strncpy(safe_filename, header->filename, 100);
        safe_filename[100] = '\0';

        size_t file_size = fk::archive::TarParser::octal_to_int(header->size, 12);
        uint8_t* data_ptr = ptr + 512;

        const char* final_filename = safe_filename;
        // Strip leading ./ or /
        if (final_filename[0] == '.' && final_filename[1] == '/') {
            final_filename += 2;
        } else if (final_filename[0] == '/') {
            final_filename += 1;
        }

        if (header->typeflag[0] == '0' || header->typeflag[0] == '\0') {
            auto file_node_res = fk::make_ref<RamFileNode>(data_ptr, file_size);
            if (!file_node_res.is_error()) {
                auto file_node = file_node_res.value();
                file_node->set_name(final_filename);
                file_node->set_parent(fk::RefPtr<Node>(this));
                m_files.push_back({final_filename, file_node});
                fk::algorithms::klog("RAMDISK", "Loaded file: %s (%zu bytes)", final_filename, file_size);
                files_loaded++;
            }
        } else if (header->typeflag[0] == '5') {
            // Directory entry in TAR - we don't strictly need to create nodes for these 
            // in our current flat RamDiskNode, but let's log it.
            fk::algorithms::klog("RAMDISK", "Found directory: %s", final_filename);
        } else if (header->typeflag[0] == '2') {
            // Symbolic link
            char link_target[100];
            strncpy(link_target, header->linkname, 100);
            link_target[99] = '\0';
            
            auto symlink_node_res = fk::make_ref<RamSymlinkNode>(link_target);
            if (!symlink_node_res.is_error()) {
                auto symlink_node = symlink_node_res.value();
                symlink_node->set_name(final_filename);
                symlink_node->set_parent(fk::RefPtr<Node>(this));
                m_files.push_back({final_filename, symlink_node});
                fk::algorithms::klog("RAMDISK", "Loaded symlink: %s -> %s", final_filename, link_target);
                files_loaded++;
            }
        }

        ptr += 512 + ((file_size + 511) & ~511u);
    }
    fk::algorithms::klog("RAMDISK", "TAR parsing complete. %zu entries loaded.", files_loaded);
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> RamDiskNode::lookup(const char* name) {
    if (!name || name[0] == '\0' || strcmp(name, ".") == 0) return fk::RefPtr<Node>(this);
    if (strcmp(name, "..") == 0) {
        if (m_parent) return m_parent;
        return fk::RefPtr<Node>(this);
    }

    const char* clean_name = (name[0] == '/') ? name + 1 : name;
    fk::algorithms::kdebug("RAMDISK", "lookup: name='%s' (clean='%s'), prefix='%s'", name, clean_name, m_prefix.c_str());
    char full_lookup[512];
    if (m_prefix.is_empty()) {
        strncpy(full_lookup, clean_name, sizeof(full_lookup) - 1);
        full_lookup[sizeof(full_lookup) - 1] = '\0';
    } else {
        // Ensure m_prefix ends with /
        if (m_prefix.c_str()[m_prefix.length() - 1] == '/') {
            snprintf(full_lookup, sizeof(full_lookup), "%s%s", m_prefix.c_str(), clean_name);
        } else {
            snprintf(full_lookup, sizeof(full_lookup), "%s/%s", m_prefix.c_str(), clean_name);
        }
    }

    for (auto& entry : m_files) {
        if (entry.name == full_lookup) {
            entry.node->set_parent(fk::RefPtr<Node>(this));
            return entry.node;
        }
    }

    char dir_prefix[512];
    if (full_lookup[0] == '\0') {
        dir_prefix[0] = '\0';
    } else {
        snprintf(dir_prefix, sizeof(dir_prefix), "%s/", full_lookup);
    }

    bool found_prefix = false;
    if (dir_prefix[0] != '\0') {
        for (auto& entry : m_files) {
            if (strncmp(entry.name.c_str(), dir_prefix, strlen(dir_prefix)) == 0) {
                found_prefix = true;
                break;
            }
        }
    }

    if (found_prefix) {
        auto dir_node_res = fk::make_ref<RamDiskNode>(m_start, m_end);
        if (dir_node_res.is_error()) return dir_node_res.error();
        auto dir_node = dir_node_res.value();
        dir_node->set_name(clean_name); // Set the component name
        for (auto& f : m_files) dir_node->m_files.push_back(f);
        dir_node->m_prefix = dir_prefix;
        dir_node->set_parent(fk::RefPtr<Node>(this));
        return fk::RefPtr<Node>(dir_node);
    }

    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error> RamDiskNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    fk::containers::Vector<fk::text::String> added_names;
    size_t prefix_len = m_prefix.length();

    fk::algorithms::klog("RAMDISK", "list_dir: prefix='%s' (len %zu), m_files size: %zu", 
                         m_prefix.c_str(), prefix_len, m_files.size());

    for (auto& entry : m_files) {
        if (prefix_len > 0 && strncmp(entry.name.c_str(), m_prefix.c_str(), prefix_len) != 0)
            continue;

        const char* relative_name = entry.name.c_str() + prefix_len;
        if (relative_name[0] == '\0') continue;

        // Get the first component of the relative name
        char component[256];
        const char* slash = strchr(relative_name, '/', 256);
        if (slash) {
            size_t len = slash - relative_name;
            if (len >= sizeof(component)) len = sizeof(component) - 1;
            strncpy(component, relative_name, len);
            component[len] = '\0';
        } else {
            strncpy(component, relative_name, sizeof(component) - 1);
            component[sizeof(component) - 1] = '\0';
        }

        // Check if already added
        bool already_added = false;
        for (auto& added : added_names) {
            if (added == component) {
                already_added = true;
                break;
            }
        }

        if (!already_added) {
            DirectoryEntry de;
            fk::memory::set(&de, 0, sizeof(de));
            strncpy(de.name, component, sizeof(de.name) - 1);
            de.name[sizeof(de.name) - 1] = '\0';
            de.type = slash ? 1 : 0; // 1 for Dir, 0 for File (simplified)
            
            fk::algorithms::klog("RAMDISK", "  entry: '%s' (type %u)", de.name, de.type);
            
            entries.push_back(de);
            added_names.push_back(component);
        }
    }
    return {};
}

} // namespace fkernel
