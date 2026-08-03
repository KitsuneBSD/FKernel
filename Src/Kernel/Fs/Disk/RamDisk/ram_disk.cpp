#include <Kernel/Fs/Disk/RamDisk/ram_disk.h>

#include <LibFK/Algorithms/Generic/container_algorithms.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/new.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<size_t, fk::core::Error> RamFileNode::read(uint64_t offset, size_t size,
                                                            uint8_t* buffer) {
  if (offset >= m_size)
    return 0;
  size_t to_read = (offset + size > m_size) ? (m_size - offset) : size;
  fk::memory::copy(buffer, m_data + offset, to_read);
  return to_read;
}

fk::core::Result<fk::RefPtr<RamDiskNode>, fk::core::Error> RamDiskNode::create(uintptr_t start,
                                                                               uintptr_t end) {
  auto res = TRY(fk::make_ref<RamDiskNode>(start, end));
  res->parse_tar();
  return fk::core::Result<fk::RefPtr<RamDiskNode>>(res);
}

void RamDiskNode::parse_tar() {
  uint8_t* ptr = reinterpret_cast<uint8_t*>(m_start);
  fk::algorithms::klog("RAMDISK", "Parsing TAR at %p - %p (size: %zu)", (void*)m_start,
                       (void*)m_end, m_end - m_start);

  size_t files_loaded = 0;
  while (ptr < reinterpret_cast<uint8_t*>(m_end)) {
    auto* header = reinterpret_cast<fk::archive::TarHeader*>(ptr);
    if (header->filename[0] == '\0') {
      fk::algorithms::klog("RAMDISK", "End of TAR found (null filename)");
      break;
    }

    // Lenient magic check: some TARs don't have ustar magic
    if (header->magic[0] != '\0' && fk::memory::compare_n(header->magic, "ustar", 5) != 0) {
      fk::algorithms::kwarn("RAMDISK", "Suspicious TAR magic at %p: '%.6s'", ptr, header->magic);
    }

    char safe_filename[101];
    fk::memory::copy_n(safe_filename, header->filename, 100);
    safe_filename[100] = '\0';

    size_t file_size = fk::archive::TarParser::octal_to_int(header->size, 12);
    uint8_t* data_ptr = ptr + 512;

    const char* final_filename = safe_filename;
    // Strip leading ./ or /
    while (final_filename[0] == '.' && final_filename[1] == '/')
      final_filename += 2;
    while (final_filename[0] == '/')
      final_filename += 1;

    if (final_filename[0] == '\0') {
      ptr += 512 + ((file_size + 511) & ~511u);
      continue;
    }

    if (header->typeflag[0] == '0' || header->typeflag[0] == '\0') {
      auto file_node_res = fk::make_ref<RamFileNode>(data_ptr, file_size);
      if (!file_node_res.is_error()) {
        auto file_node = file_node_res.value();

        const char* name_ptr = strrnchr(final_filename, '/', 100);
        if (name_ptr)
          name_ptr++;
        else
          name_ptr = final_filename;

        file_node->set_name(name_ptr);
        file_node->set_parent(fk::RefPtr<Node>(this));
        m_files.push_back({final_filename, file_node});
        fk::algorithms::klog("RAMDISK", "Loaded file: %s (%zu bytes) as '%s'", final_filename,
                             file_size, name_ptr);
        files_loaded++;
      }
    } else if (header->typeflag[0] == '5') {
      // Explicit directory entry
      // Strip trailing slash if present for consistency
      size_t len = fk::memory::length(final_filename);
      char temp[256];
      if (len > 0 && final_filename[len - 1] == '/') {
        fk::memory::copy_n(temp, final_filename, len - 1);
        temp[len - 1] = '\0';
      } else {
        fk::memory::copy_n(temp, final_filename, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
      }
      m_directories.push_back(temp);
      fk::algorithms::klog("RAMDISK", "Found explicit directory: %s", temp);
    } else if (header->typeflag[0] == '2') {
      // Symbolic link
      char link_target[100];
      fk::memory::copy_n(link_target, header->linkname, 100);
      link_target[99] = '\0';

      auto symlink_node_res = fk::make_ref<RamSymlinkNode>(link_target);
      if (!symlink_node_res.is_error()) {
        auto symlink_node = symlink_node_res.value();

        const char* name_ptr = strrnchr(final_filename, '/', 100);
        if (name_ptr)
          name_ptr++;
        else
          name_ptr = final_filename;

        symlink_node->set_name(name_ptr);
        symlink_node->set_parent(fk::RefPtr<Node>(this));
        m_files.push_back({final_filename, symlink_node});
        fk::algorithms::klog("RAMDISK", "Loaded symlink: %s -> %s as '%s'", final_filename,
                             link_target, name_ptr);
        files_loaded++;
      }
    }

    ptr += 512 + ((file_size + 511) & ~511u);
  }
  fk::algorithms::klog("RAMDISK", "TAR parsing complete. %zu entries loaded.", files_loaded);
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> RamDiskNode::lookup(const char* name) {
  if (!name || name[0] == '\0' || fk::memory::compare(name, ".") == 0)
    return fk::RefPtr<Node>(this);
  if (fk::memory::compare(name, "..") == 0) {
    if (m_parent)
      return m_parent;
    return fk::RefPtr<Node>(this);
  }

  const char* clean_name = (name[0] == '/') ? name + 1 : name;
  char full_lookup[512];
  if (m_prefix.is_empty()) {
    fk::memory::copy_n(full_lookup, clean_name, sizeof(full_lookup) - 1);
  } else {
    snprintf(full_lookup, sizeof(full_lookup), "%s%s", m_prefix.c_str(), clean_name);
  }
  full_lookup[sizeof(full_lookup) - 1] = '\0';

  for (auto& entry : m_files) {
    if (entry.name == full_lookup) {
      entry.node->set_parent(fk::RefPtr<Node>(this));
      return entry.node;
    }
  }

  // Check if this is an implied directory (it's a prefix for other files)
  char dir_prefix[512];
  snprintf(dir_prefix, sizeof(dir_prefix), "%s/", full_lookup);

  bool found_prefix = false;
  for (auto& entry : m_files) {
    if (fk::memory::compare_n(entry.name.c_str(), dir_prefix, fk::memory::length(dir_prefix)) == 0) {
      found_prefix = true;
      break;
    }
  }

  bool is_explicit_dir = false;
  for (const auto& dir : m_directories) {
    if (dir == full_lookup) {
      is_explicit_dir = true;
      break;
    }
  }

  if (found_prefix || is_explicit_dir) {
    auto dir_node_res = fk::make_ref<RamDiskNode>(m_start, m_end);
    if (dir_node_res.is_error())
      return dir_node_res.error();
    auto dir_node = dir_node_res.value();
    dir_node->set_name(clean_name);

    // Copy m_files and m_directories to the new directory node so it can find its own children
    for (auto& f : m_files)
      dir_node->m_files.push_back(f);
    for (auto& d : m_directories)
      dir_node->m_directories.push_back(d);

    dir_node->m_prefix = dir_prefix;
    dir_node->set_parent(fk::RefPtr<Node>(this));
    return fk::RefPtr<Node>(dir_node);
  }

  return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
RamDiskNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  size_t prefix_len = m_prefix.length();

  for (auto& entry : m_files) {
    if (prefix_len > 0 && fk::memory::compare_n(entry.name.c_str(), m_prefix.c_str(), prefix_len) != 0)
      continue;

    const char* relative_name = entry.name.c_str() + prefix_len;
    if (relative_name[0] == '\0')
      continue;

    char component[256];
    const char* slash = strnchr(relative_name, '/', 256);
    bool is_dir = false;

    if (slash) {
      size_t len = slash - relative_name;
      if (len == 0) {
        relative_name++;
        slash = strnchr(relative_name, '/', 256);
        if (slash) {
          len = slash - relative_name;
          fk::memory::copy_n(component, relative_name, len);
          component[len] = '\0';
          is_dir = true;
        } else {
          fk::memory::copy_n(component, relative_name, sizeof(component) - 1);
          is_dir = false;
        }
      } else {
        fk::memory::copy_n(component, relative_name, len);
        component[len] = '\0';
        is_dir = true;
      }
    } else {
      fk::memory::copy_n(component, relative_name, sizeof(component) - 1);
      component[sizeof(component) - 1] = '\0';
      is_dir = false;
    }
    if (fk::algorithms::find_if(entries.begin(), entries.size(),
          [&](const DirectoryEntry& e) { return fk::memory::compare(e.name, component) == 0; })
        == entries.size()) {
      DirectoryEntry de;
      fk::memory::set(&de, 0, sizeof(de));
      fk::memory::copy_n(de.name, component, sizeof(de.name) - 1);
      if (is_dir) {
        de.type = 1; // DT_DIR
      } else {
        de.type = entry.node->is_symlink() ? 2 : 0; // DT_LNK : DT_REG
      }
      entries.push_back(de);
    }
  }

  // Add explicit directories that might be empty (not implied by files)
  for (const auto& dir_path : m_directories) {
    if (prefix_len > 0 && fk::memory::compare_n(dir_path.c_str(), m_prefix.c_str(), prefix_len) != 0)
      continue;

    const char* relative_name = dir_path.c_str() + prefix_len;
    if (relative_name[0] == '\0')
      continue;

    char component[256];
    const char* slash = strnchr(relative_name, '/', 256);

    if (slash) {
      // Should not happen if m_directories contains clean paths, but safe to handle
      size_t len = slash - relative_name;
      fk::memory::copy_n(component, relative_name, len);
      component[len] = '\0';
    } else {
      fk::memory::copy_n(component, relative_name, sizeof(component) - 1);
      component[sizeof(component) - 1] = '\0';
    }

    if (fk::algorithms::find_if(entries.begin(), entries.size(),
          [&](const DirectoryEntry& e) { return fk::memory::compare(e.name, component) == 0; })
        == entries.size()) {
      DirectoryEntry de;
      fk::memory::set(&de, 0, sizeof(de));
      fk::memory::copy_n(de.name, component, sizeof(de.name) - 1);
      de.type = 1; // DT_DIR
      entries.push_back(de);
    }
  }

  return {};
}

} // namespace fkernel
