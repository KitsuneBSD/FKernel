#pragma once

#include <Kernel/FileSystem/VirtualFS/VNodeType.h>
#include <Kernel/FileSystem/VirtualFS/VNodeFlags.h>
#include <Kernel/FileSystem/VirtualFS/VNodeOps.h>

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Memory/own_ptr.h>

/**
 * @brief Virtual Node (VNode) structure.
 *
 * Represents a node in a virtual filesystem.
 * Can be a file, directory,symlink, or device.
 */
struct VNode
{
    VNodeType m_type;                                  ///< Type of node
    fixed_string<256> m_name;                          ///< Name of node
    RetainPtr<VNode> m_parent;                         ///< Pointer to parent vnode
    static_vector<RetainPtr<VNode>, 65535> m_children; ///< Children (only for directories)
    uint32_t m_flags;

    void *m_data{nullptr}; ///< Generic Pointer about file data
    uint64_t m_size{0};    ///< Size in Bytes (for files)
    uint64_t m_inode{0};   ///< Unique inode number

    VNodeOps *ops = nullptr; ///< Pointer to Vnode Operations

    /// @brief Constructor for files, directories, or symlinks
    VNode(const char *name, VNodeType t, RetainPtr<VNode> parent, uint32_t flags = VNodeFlags::NONE)
        : m_type(t), m_name(name), m_parent(parent), m_flags{flags} {}

    /// @brief Add a child node (directory only)
    bool add_child(RetainPtr<VNode> child)
    {
        if (m_type != VNodeType::Directory)
            return false;
        return m_children.push_back(move(child));
    }

    /// @brief Remove a child by index
    void remove_child(size_t index)
    {
        if (m_type != VNodeType::Directory)
            return;
        m_children.erase(index);
    }

    /// @brief Get child by name
    VNode *find_child(const char *child_name)
    {
        for (size_t i = 0; i < m_children.size(); ++i)
        {
            if (strcmp(m_children[i]->m_name.c_str(), child_name) == 0)
                return m_children[i].get();
        }
        return nullptr;
    }

    /// @brief Retain / Release interface for RetainPtr
    void retain() {}
    void release() {}
};