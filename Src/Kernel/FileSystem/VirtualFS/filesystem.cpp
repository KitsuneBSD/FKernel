#include <Kernel/FileSystem/VirtualFS/filesystem.h>

namespace fkernel::fs {

// Define the static member for filesystem drivers
fk::containers::static_vector<Filesystem::ProbeFunction, 8>
    Filesystem::s_filesystem_drivers;

} // namespace fkernel::fs
