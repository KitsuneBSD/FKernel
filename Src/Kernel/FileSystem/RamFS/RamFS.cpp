#include <Kernel/FileSystem/RamFS/RamFS.h>
#include <Kernel/FileSystem/VirtualFS/VNode.h>

#include <Kernel/MemoryManager/TlsfHeap.h>
#include <LibC/stdlib.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

ssize_t ramfs_read(VNode *node, void *buffer, size_t size, size_t offset)
{
    if (!node || !buffer || offset >= node->m_size)
    {
        kwarn("RAMFS", "Read failed: invalid node or buffer, or offset out of bounds");
        return 0;
    }

    size_t to_read = (offset + size > node->m_size) ? (node->m_size - offset) : size;
    memcpy(buffer, (uint8_t *)node->m_data + offset, to_read);

    klog("RAMFS", "Read %lu bytes from '%s' at offset %lu", to_read, node->m_name.c_str(), offset);
    return to_read;
}

ssize_t ramfs_write(VNode *node, const void *buffer, size_t size, size_t offset)
{
    if (!node || !buffer)
    {
        kwarn("RAMFS", "Write failed: invalid node or buffer");
        return -1;
    }

    size_t new_size = offset + size;
    if (new_size > node->m_size)
    {
        node->m_data = realloc(node->m_data, new_size);
        node->m_size = new_size;
        klog("RAMFS", "Expanded node '%s' to %lu bytes", node->m_name.c_str(), node->m_size);
    }

    memcpy((uint8_t *)node->m_data + offset, buffer, size);
    klog("RAMFS", "Wrote %lu bytes to '%s' at offset %lu", size, node->m_name.c_str(), offset);
    return size;
}

int ramfs_open(VNode *node)
{
    klog("RAMFS", "Opened node '%s'", node ? node->m_name.c_str() : "(null)");
    return 0;
}

int ramfs_close(VNode *node)
{
    klog("RAMFS", "Closed node '%s'", node ? node->m_name.c_str() : "(null)");
    return 0;
}

VNode *ramfs_lookup(VNode *node, const char *name)
{
    VNode *child = node ? node->find_child(name) : nullptr;
    klog("RAMFS", "Lookup for '%s' in '%s' %s",
         name,
         node ? node->m_name.c_str() : "(null)",
         child ? "found" : "not found");
    return child;
}

VNode *ramfs_create(VNode *node, const char *name, VNodeType type)
{
    if (!node || node->m_type != VNodeType::Directory || !name)
    {
        kwarn("RAMFS", "Create failed: invalid parent or name");
        return nullptr;
    }

    if (node->find_child(name))
    {
        kwarn("RAMFS", "Create failed: node '%s' already exists under '%s'", name, node->m_name.c_str());
        return nullptr;
    }

    RetainPtr<VNode> child(new VNode(name, type, node));
    child->ops = &ramfs_ops;

    if (!node->add_child(move(child)))
    {
        kwarn("RAMFS", "Failed to add child '%s' to '%s'", name, node->m_name.c_str());
        return nullptr;
    }

    klog("RAMFS", "Created node '%s' under '%s'", name, node->m_name.c_str());
    return node->find_child(name);
}

int ramfs_remove(VNode *node, VNode *child)
{
    if (!node || node->m_type != VNodeType::Directory || !child)
    {
        kwarn("RAMFS", "Remove failed: invalid parent or child");
        return -1;
    }

    for (size_t i = 0; i < node->m_children.size(); ++i)
    {
        if (node->m_children[i].get() == child)
        {
            if (child->m_data)
            {
                free(child->m_data);
                child->m_data = nullptr;
                child->m_size = 0;
            }

            node->remove_child(i);
            klog("RAMFS", "Removed node '%s' from '%s'", child->m_name.c_str(), node->m_name.c_str());
            return 0;
        }
    }

    kwarn("RAMFS", "Remove failed: node '%s' not found under '%s'", child->m_name.c_str(), node->m_name.c_str());
    return -1;
}

VNodeOps ramfs_ops = {
    .read = ramfs_read,
    .write = ramfs_write,
    .open = ramfs_open,
    .close = ramfs_close,
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .remove = ramfs_remove};

RetainPtr<VNode> ramfs_init()
{
    RetainPtr<VNode> root(new VNode("/", VNodeType::Directory));
    root->ops = &ramfs_ops;
    klog("RAMFS", "Initialized RAMFS root '/'");
    return root;
}
