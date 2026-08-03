#pragma once

#include <LibFK/Container/Sequence/static_vector.h>
#include <LibFK/Text/fixed_string.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Mount/mount_namespace.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>

static constexpr size_t MAX_OPEN_FILES = 1024;

struct TaskFiles {
  fk::text::fixed_string<256> cwd{"/"};
  fk::text::fixed_string<512> exe_path{};
  fk::RefPtr<fkernel::Dentry> root;
  fk::RefPtr<fkernel::MountNamespace> mount_ns;
  fk::containers::static_vector<fk::RefPtr<FileDescription>, MAX_OPEN_FILES> descriptors;
  fk::containers::static_vector<uint32_t, MAX_OPEN_FILES> cap_handles;
};
