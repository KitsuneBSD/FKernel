#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Text/fixed_string.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Mount/mount_namespace.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>

// Soft per-process FD limit (matches RLIMIT_NOFILE default).
// Not a hard cap: install_at() can still grow beyond this with explicit fd numbers.
static constexpr int SOFT_OPEN_FILES_LIMIT = 65536;

struct TaskFiles {
  fk::text::fixed_string<256> cwd{"/"};
  fk::text::fixed_string<512> exe_path{};
  fk::RefPtr<fkernel::Dentry> root;
  fk::RefPtr<fkernel::MountNamespace> mount_ns;
  fk::containers::Vector<fk::RefPtr<FileDescription>> descriptors;
  fk::containers::Vector<uint32_t> cap_handles;
};
