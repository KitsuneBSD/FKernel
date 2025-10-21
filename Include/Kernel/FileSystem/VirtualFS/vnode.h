#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode_type.h>
#include <Kernel/FileSystem/VirtualFS/vnode_flags.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/inode.h>
#include <Kernel/FileSystem/VirtualFS/dir_entry.h>

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Memory/own_ptr.h>

struct VNodeOps;

/**
 * @brief Virtual Node (VNode) structure.
 *
 * Represents a node in a virtual filesystem.
 * Can be a file, directory,symlink, or device.
 */
struct VNode
{
private:
    uint64_t m_refcount{1};

public:
    fixed_string<64> m_name;            ///< Node name
    VNodeType type{VNodeType::Unknown}; ///< Node type
    uint32_t permission{0};             ///< Node permissions
    uint64_t size{0};                   ///< Node size in bytes
    uint64_t inode_number{0};           ///< INode number

    RetainPtr<VNode> parent;   ///< Parent number
    VNodeOps *ops{nullptr};    ///< Operations Table
    void *fs_private{nullptr}; ///< FileSystem expecific private data

    Inode *inode;
    // TODO: Change static_vector to dynamic_vector
    static_vector<DirEntry, 16> dir_entries;

    void retain() { ++m_refcount; }
    void release()
    {
        if (--m_refcount == 0)
            delete this;
    }

    /**
     * @brief Read from the vnode.
     * @param buf Destination buffer.
     * @param sz Number of bytes to read.
     * @param off Offset in the file.
     * @return Number of bytes read, or negative on error.
     */
    inline int read(void *buf, size_t sz, size_t off)
    {
        int ret = ops && ops->read ? ops->read(this, nullptr, buf, sz, off) : -1;
        if (ret >= 0)
            klog("VFS", "Read %d bytes from '%s' at offset %zu", ret, m_name.c_str(), off);
        else
            kwarn("VFS", "Failed to read from '%s'", m_name.c_str());
        return ret;
    }

    /**
     * @brief Write to the vnode.
     * @param buf Source buffer.
     * @param sz Number of bytes to write.
     * @param off Offset in the file.
     * @return Number of bytes written, or negative on error.
     */
    inline int write(const void *buf, size_t sz, size_t off)
    {
        int ret = ops && ops->write ? ops->write(this, nullptr, buf, sz, off) : -1;
        if (ret >= 0)
            klog("VFS", "Wrote %d bytes to '%s' at offset %zu", ret, m_name.c_str(), off);
        else
            kwarn("VFS", "Failed to write to '%s'", m_name.c_str());
        return ret;
    }

    /**
     * @brief Open the vnode.
     * @param flags Open flags.
     * @return 0 on success or negative on error.
     */
    inline int open(int flags)
    {
        int ret = ops && ops->open ? ops->open(this, nullptr, flags) : 0;
        if (ret == 0)
            klog("VFS", "Opened '%s' with flags 0x%x", m_name.c_str(), flags);
        else
            kwarn("VFS", "Failed to open '%s' with flags 0x%x", m_name.c_str(), flags);
        return ret;
    }

    /**
     * @brief Close the vnode.
     * @return 0 on success or negative on error.
     */
    inline int close()
    {
        int ret = ops && ops->close ? ops->close(this, nullptr) : 0;
        if (ret == 0)
            klog("VFS", "Closed '%s'", m_name.c_str());
        else
            kwarn("VFS", "Failed to close '%s'", m_name.c_str());
        return ret;
    }

    /**
     * @brief Lookup a child vnode by name.
     * @param name Name of the child.
     * @param out Output vnode pointer.
     * @return 0 on success or negative on error.
     */
    inline int lookup(const char *name, RetainPtr<VNode> &out)
    {
        if (type != VNodeType::Directory)
        {
            kwarn("VFS", "Lookup failed on non-directory '%s'", m_name.c_str());
            return -1;
        }

        for (size_t i = 0; i < dir_entries.size(); ++i)
        {
            if (strcmp(dir_entries[i].m_name.c_str(), name) == 0)
            {
                out = dir_entries[i].m_vnode;
                klog("VFS", "Lookup: found '%s' in directory '%s'", name, m_name.c_str());
                return 0;
            }
        }

        kwarn("VFS", "Lookup failed: '%s' not found in directory '%s'", name, m_name.c_str());
        return -1;
    }

    /**
     * @brief Create a new child vnode.
     * @param name Name of the new vnode.
     * @param t Type of the new vnode.
     * @param out Output vnode pointer.
     * @return 0 on success or negative on error.
     */
    inline int create(const char *name, VNodeType t, RetainPtr<VNode> &out)
    {
        if (type != VNodeType::Directory)
        {
            kwarn("VFS", "Create failed: '%s' is not a directory", m_name.c_str());
            return -1;
        }

        Inode *new_inode = new Inode(inode_number + 1);
        VNode *child = new VNode();
        child->m_name = name;
        child->type = t;
        child->parent = this;
        child->inode = new_inode;
        child->inode_number = new_inode->i_number;

        dir_entries.push_back(DirEntry{name, adopt_retain(child)});
        out = adopt_retain(child);

        klog("VFS", "Created '%s' in directory '%s'", name, m_name.c_str());
        return 0;
    }
};