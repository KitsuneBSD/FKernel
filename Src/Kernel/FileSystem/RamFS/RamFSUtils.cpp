#include <Kernel/FileSystem/RamFS/RamFSUtils.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/string.h>
#include <LibFK/enforce.h>

namespace FileSystem {

bool ramfs_node_expand_data(RamFSNode* node, LibC::size_t new_size)
{
    if (FK::alert_if(new_size <= node->data_capacity, "RAMFS: new_size is minor or equal to ramfs node capacity"))
        return true;

    LibC::size_t alloc_size = (new_size > node->data_capacity * 2) ? new_size : node->data_capacity;

    if (alloc_size < 64)
        alloc_size = 64;

    void* new_buffer = Falloc(alloc_size);

    if (FK::alert_if(!new_buffer, "RAMFS: Fail to alloc memory buffer"))
        return false;

    if (node->data) {
        LibC::memcpy(new_buffer, node->data, node->data_size);
    }

    node->data = static_cast<LibC::uint8_t*>(new_buffer);
    node->data_capacity = alloc_size;
    return true;
}

void ramfs_node_free_data(RamFSNode* node)
{
    if (node->data) {
        node->data = nullptr;
        node->data_size = 0;
        node->data_capacity = 0;
    }
}

void ramfs_add_child(RamFSNode* parent, RamFSNode* child)
{
    child->parent = parent;
    parent->children.append(child);
}

}
