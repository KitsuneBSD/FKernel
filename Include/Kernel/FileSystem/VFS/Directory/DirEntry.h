#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/stddef.h>

namespace FileSystem {

constexpr LibC::size_t MAX_FILENAME_LEN = 255;

struct DirEntry {
    char name[MAX_FILENAME_LEN + 1];
    VNode* vnode;
    VNodeType type;

    DirEntry()
        : vnode(nullptr)
        , type(VNodeType::Unknown)
    {
        name[0] = '\0';
    }

    DirEntry(char const* new_name, VNode* new_vnode, VNodeType new_type)
        : vnode(new_vnode)
        , type(new_type)
    {
        if (new_name) {
            LibC::size_t len = LibC::strlen(new_name);
            if (len > MAX_FILENAME_LEN)
                len = MAX_FILENAME_LEN;
            for (LibC::size_t i = 0; i < len; ++i)
                name[i] = new_name[i];
            name[len] = '\0';
        } else {
            name[0] = '\0';
        }
    }

    [[nodiscard]] bool equals_name(char const* other_name) const
    {
        return LibC::strcmp(name, other_name) == 0;
    }
};
}
