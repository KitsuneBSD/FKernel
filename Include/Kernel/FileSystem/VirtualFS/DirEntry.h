#pragma once

#include <LibFK/Container/fixed_string.h>
#include <LibC/stdint.h>

struct DirEntry
{
    fixed_string<256> m_name;
    uint64_t i_number;

    DirEntry() : m_name(""), i_number(0) {}

    DirEntry(const char *name, uint64_t inode_num)
        : m_name(name), i_number(inode_num) {}
};