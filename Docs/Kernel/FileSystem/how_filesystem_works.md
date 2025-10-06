# FKernel Filesystem Documentation: VFS

## Overview

The FKernel filesystem is built on top of a **Virtual Filesystem (VFS)** abstraction.  
VFS provides a unified interface to access multiple types of filesystems (RAMFS, DiskFS, devices, etc.), making the kernel extensible and modular.

### Goals

- Enable multiple filesystem types under a single interface
- Simplify file and directory management
- Provide a clear API for userland and kernel modules

---

## VNode: Virtual Node

`VNode` represents a file, directory, symlink, or device in the VFS.

```cpp
struct VNode
{
    VNodeType m_type;                     // Node type: file, directory, symlink, device
    fixed_string<256> m_name;             // Node name
    RetainPtr<VNode> m_parent;            // Pointer to parent vnode
    static_vector<RetainPtr<VNode>, 65535> m_children; // Children (directories only)
    uint32_t m_flags;                     // Node flags
    void *m_data{nullptr};                // File content pointer
    uint64_t m_size{0};                   // File size in bytes
    uint64_t m_inode{0};                  // Unique inode number
    VNodeOps *ops = nullptr;              // Pointer to operations
};
```

### Key Features

- Directories: m_children stores all child nodes.
- Files: m_data stores file content in memory.
- Reference Management: RetainPtr handles memory and reference counting automatically.
- VNodeOps: Defines filesystem-specific behavior.

### VNode Operations (VNodeOps)

VNodeOps defines all operations that a filesystem can perform on a VNode.

```cpp
struct VNodeOps
{
    ssize_t (*read)(VNode *node, void *buffer, size_t size, size_t offset);
    ssize_t (*write)(VNode *node, const void *buffer, size_t size, size_t offset);
    int (*open)(VNode *node);
    int (*close)(VNode *node);
    VNode *(*lookup)(VNode *node, const char *name);
    VNode *(*create)(VNode *node, const char *name, VNodeType type);
    int (*remove)(VNode *node, VNode *child);
};
```

### Standard Operations

- `read` / `write`: Access file contents.

- `open` / `close`: Open or close files (optional for in-memory filesystems).

- `lookup`: Find child nodes in a directory.

- `create` / `remove`: Modify directory contents

### Filesystem Types

FKernel supports multiple types of filesystems, each implementing its own `VNodeOps`.

| Filesystem   | Description |
|-------------|-------------|
| **RAMFS**   | In-memory filesystem, suitable for temporary files and pseudo-filesystems. |
| **DiskFS**  | Persistent filesystem stored on disk or block devices. |
| **DeviceFS**| Special filesystem exposing hardware devices as VNodes. |

---

## Example Usage

### Creating a Directory and File in RAMFS

```cpp
// Create the root directory
RetainPtr<VNode> root = make_retain<VNode>("root", VNodeType::Directory, nullptr);

// Assign RAMFS operations
VNodeOps ramfs_ops = {
    .read = ramfs_read,
    .write = ramfs_write,
    .open = ramfs_open,
    .close = ramfs_close,
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .remove = ramfs_remove
};
root->ops = &ramfs_ops;

// Create a file in the root directory
VNode *file = root->ops->create(root.get(), "hello.txt", VNodeType::File);
root->add_child(make_retain(file));

// Write content to the file
const char *msg = "Hello, FKernel!";
file->ops->write(file, msg, strlen(msg), 0);
```

### Mount Points
- Future versions of FKernel may allow mounting different filesystems at arbitrary paths.
- Mount points provide seamless integration of multiple filesystems under a single unified namespace.

### Permission and Ownership

Currently, filesystems do not enforce access control.

Future extensions can include:

- User and group ownership

- Read/write/execute permissions

- Special flags for system files 