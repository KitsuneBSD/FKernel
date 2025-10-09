#pragma once

#include <LibFK/Container/fixed_string.h>
#include <LibC/stdint.h>

/**
 * @brief Represents a directory entry in the filesystem.
 *
 * A DirEntry associates a filename with an inode number.
 */
struct DirEntry
{
    fixed_string<256> m_name; ///< Name of the file or directory
    uint64_t i_number;        ///< Corresponding inode number

    /**
     * @brief Default constructor. Initializes name to empty and inode number to 0.
     */
    DirEntry() : m_name(""), i_number(0) {}

    /**
     * @brief Constructs a DirEntry with a given name and inode number.
     *
     * @param name Name of the file or directory
     * @param inode_num Associated inode number
     */
    DirEntry(const char *name, uint64_t inode_num)
        : m_name(name), i_number(inode_num) {}
};