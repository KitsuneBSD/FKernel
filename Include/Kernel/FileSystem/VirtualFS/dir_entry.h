#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Types/types.h>

struct VNode;

/**
 * @brief Represents a directory entry in the filesystem.
 *
 * A DirEntry associates a filename with an inode number.
 */
struct DirEntry {
  fixed_string<256> m_name; ///< Name of the file or directory
  RetainPtr<VNode> m_vnode; ///< Vnode of directory

  /**
   * @brief Default constructor. Initializes name to empty and inode number to
   * 0.
   */
  DirEntry() : m_name(""), m_vnode(nullptr) {}

  /**
   * @brief Constructs a DirEntry with a given name and inode number.
   *
   * @param name Name of the file or directory
   * @param vnode pointer to equivalent vnode
   */
  DirEntry(const char *name, RetainPtr<VNode> vnode)
      : m_name(name), m_vnode(vnode) {}
};
