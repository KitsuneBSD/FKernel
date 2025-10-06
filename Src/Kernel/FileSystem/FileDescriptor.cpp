#include <Kernel/FileSystem/FileDescriptor.h>

void file_ref(struct file *f)
{
    if (f)
        f->refcount++;
}

void file_unref(struct file *f)
{
    if (f && --f->refcount == 0)
    {
    }
}

void filedesc_init(struct file_desc *fdp)
{
    fdp->fd_count = 0;
    for (int i = 0; i < MAX_FD; ++i)
    {
        fdp->fd_files[i].file = nullptr;
        fdp->fd_files[i].flags = 0;
    }
}

int filedesc_alloc(struct file_desc *fdp, struct file *fp, int flags)
{
    for (int i = 0; i < MAX_FD; ++i)
    {
        if (fdp->fd_files[i].file == nullptr)
        {
            fdp->fd_files[i].file = fp;
            fdp->fd_files[i].flags = flags;

            if (fp)
            {
                file_ref(fp);
            }

            if (i >= fdp->fd_count)
            {
                fdp->fd_count += 1;
            }

            return i;
        }
    }

    return NO_FD;
}

struct file *filedesc_get(struct file_desc *fdp, int fd)
{
    if (fd < 0 || fd >= MAX_FD)
    {
        return NULL;
    }
    return fdp->fd_files[fd].file;
}

void filedesc_close(struct file_desc *fdp, int fd)
{
    if (fd < 0 || fd >= MAX_FD)
        return;
    if (fdp->fd_files[fd].file)
    {
        file_unref(fdp->fd_files[fd].file);
        fdp->fd_files[fd].file = NULL;
        fdp->fd_files[fd].flags = 0;
    }
}

int filedesc_dup(struct file_desc *fdp, int oldfd)
{
    if (oldfd < 0 || oldfd >= MAX_FD)
        return NO_FD;
    struct file *fp = fdp->fd_files[oldfd].file;
    if (!fp)
        return NO_FD;
    file_ref(fp);
    return filedesc_alloc(fdp, fp, fdp->fd_files[oldfd].flags);
}