#pragma once

#include <LibFK/Types/types.h>

#include <Kernel/Fs/Vfs/Definitions/file_descriptor.h>
#include <Kernel/Fs/Vfs/Definitions/inode_index.h>
#include <Kernel/Fs/Vfs/Definitions/seek_mode.h>

// Standard POSIX-like flags
static constexpr int O_RDONLY = 0;
static constexpr int O_WRONLY = 1;
static constexpr int O_RDWR = 2;
static constexpr int O_CREAT = 64;
static constexpr int O_EXCL = 128;
static constexpr int O_NOCTTY = 256;
static constexpr int O_TRUNC = 512;
static constexpr int O_APPEND = 1024;
static constexpr int O_NONBLOCK = 2048;
static constexpr int O_DIRECTORY = 0x10000;
static constexpr int O_CLOEXEC = 0x80000;
static constexpr int O_LARGEFILE = 0x8000;
static constexpr int O_ACCMODE = 3;

struct linux_dirent {
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    char     d_name[];
};

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    uint16_t d_reclen;
    uint8_t  d_type;
    char     d_name[];
};

struct [[gnu::packed]] bsd_dirent {
    uint64_t d_ino;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[];
};

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
