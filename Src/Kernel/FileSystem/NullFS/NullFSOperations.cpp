#include <Kernel/FileSystem/NullFS/NullFSOperations.h>
#include <Kernel/Posix/errno.h>

namespace FileSystem {

int nullfs_open(VNode*, LibC::uint32_t)
{
    return 0;
}

LibC::ssize_t nullfs_read(VNode*, LibC::uint64_t, void*, LibC::size_t)
{
    return 0; // EOF
}

LibC::ssize_t nullfs_write(VNode*, LibC::uint64_t, void const*, LibC::size_t size)
{
    return static_cast<LibC::ssize_t>(size); // DevNull descarta tudo
}

int nullfs_close(VNode*)
{
    return 0;
}

int nullfs_stat(VNode*, FileStat* stat)
{
    stat->type = VNodeType::Device;
    stat->size = 0;
    stat->permissions = 0666;
    stat->inode = 1;
    stat->uid = 0;
    stat->gid = 0;
    return 0;
}

int nullfs_mkdir(VNode*, char const*, LibC::uint32_t)
{
    return -EACCES;
}

int nullfs_unlink(VNode*, char const*)
{
    return -EACCES;
}

int nullfs_rename(VNode*, char const*, char const*)
{
    return -EINVAL;
}

int nullfs_symlink(VNode*, char const*, char const*)
{
    return -EACCES;
}

LibC::ssize_t nullfs_readlink(VNode*, char* buf, LibC::size_t bufsize)
{
    if (bufsize > 0)
        buf[0] = '\0';
    return 0;
}

int nullfs_chmod(VNode*, LibC::uint32_t)
{
    return 0; // Ignorado
}

int nullfs_chown(VNode*, LibC::uint32_t, LibC::uint32_t)
{
    return 0; // Ignorado
}

// Não há diretório ou filhos
int nullfs_readdir(VNode*, LibC::uint64_t, char*, VNode**)
{
    return -ENOTDIR;
}

VNode* nullfs_lookup(VNode*, char const*)
{
    return nullptr; // Sem filhos
}

VNode* nullfs_create(VNode*, char const*, VNodeType)
{
    return nullptr; // Não suportado
}

}
