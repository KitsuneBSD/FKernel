#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

struct Inode
{
    uint64_t i_number;
    uint32_t permissions;
    uint64_t size;
    uint64_t link_count;
    uint64_t creation_time;
    uint64_t modification_time;
    uint64_t access_time;
    uint64_t data_block_pointers[12];

    Inode(uint64_t inode_num)
        : i_number(inode_num), permissions(0), size(0), link_count(1),
          creation_time(0), modification_time(0), access_time(0)
    {
        for (size_t i = 0; i < 12; ++i)
            data_block_pointers[i] = 0;
    }
};