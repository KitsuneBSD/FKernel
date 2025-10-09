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

struct VNodeOps;

/**
 * @brief Virtual Node (VNode) structure.
 *
 * Represents a node in a virtual filesystem.
 * Can be a file, directory,symlink, or device.
 */
struct VNode
{
    fixed_string<64> m_name;            ///< Node name
    VNodeType type{VNodeType::Unknown}; ///< Node type
    uint32_t permission{0};             ///< Node permissions
    uint64_t size{0};                   ///< Node size in bytes
    uint64_t inode{0};                  ///< INode number

    RetainPtr<VNode> parent;      ///< Parent number
    const VNodeOps *ops{nullptr}; ///< Operations Table
    void *fs_private{nullptr};    ///< FileSystem expecific private data

    /**
     * @brief Read from the vnode.
     * @param buf Destination buffer.
     * @param sz Number of bytes to read.
     * @param off Offset in the file.
     * @return Number of bytes read, or negative on error.
     */
    inline int read(void *buf, size_t sz, size_t off)
    {
        return ops && ops->read ? ops->read(this, buf, sz, off) : -1;
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
        return ops && ops->write ? ops->write(this, buf, sz, off) : -1;
    }

    /**
     * @brief Open the vnode.
     * @param flags Open flags.
     * @return 0 on success or negative on error.
     */
    inline int open(int flags)
    {
        return ops && ops->open ? ops->open(this, flags) : 0;
    }

    /**
     * @brief Close the vnode.
     * @return 0 on success or negative on error.
     */
    inline int close()
    {
        return ops && ops->close ? ops->close(this) : 0;
    }

    /**
     * @brief Lookup a child vnode by name.
     * @param name Name of the child.
     * @param out Output vnode pointer.
     * @return 0 on success or negative on error.
     */
    inline int lookup(const char *name, RetainPtr<VNode> &out)
    {
        return ops && ops->lookup ? ops->lookup(this, name, out) : -1;
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
        return ops && ops->create ? ops->create(this, name, t, out) : -1;
    }
};