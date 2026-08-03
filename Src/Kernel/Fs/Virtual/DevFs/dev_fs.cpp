#include <Kernel/Fs/Virtual/DevFs/dev_fs.h>
#include <LibFK/Algorithms/Generic/container_algorithms.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

DevFs::DevFs() {
    // Vector will be pre-allocated as needed
}

DevFs& DevFs::the() {
    static fk::RefPtr<DevFs> instance = fk::make_ref<DevFs>().value();
    return *instance;
}

fk::core::Result<void, fk::core::Error> DevFs::register_device(fk::RefPtr<Node> node, const char* name) {
    if (!node || !name) return fk::core::Error::InvalidParameter;

    fk::synchronization::ScopedLockIRQ lock(m_lock);
    size_t idx = fk::algorithms::find_if(m_devices.begin(), m_devices.size(),
        [&name](const auto& e) { return e.name == name; });
    if (idx != m_devices.size()) return fk::core::Error::PermissionDenied;

    m_devices.push_back({name, node});
    fk::algorithms::klog("DEVFS", "Registered device: /dev/%s", name);
    return {};
}

fk::core::Result<void, fk::core::Error> DevFs::unregister_device(const char* name) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    size_t idx = fk::algorithms::find_if(m_devices.begin(), m_devices.size(),
        [&name](const auto& e) { return e.name == name; });
    if (idx == m_devices.size()) return fk::core::Error::NotFound;
    m_devices[idx] = fk::types::move(m_devices[m_devices.size() - 1]);
    m_devices.pop_back();
    return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> DevFs::lookup(const char* name) {
    if (!name) return fk::core::Error::InvalidParameter;

    fk::synchronization::ScopedLockIRQ lock(m_lock);
    size_t idx = fk::algorithms::find_if(m_devices.begin(), m_devices.size(),
        [&name](const auto& e) { return e.node && fk::memory::compare(e.name.c_str(), name) == 0; });
    if (idx != m_devices.size()) return m_devices[idx].node;
    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error> DevFs::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    for (auto& entry : m_devices) {
        if (!entry.node) continue;
        
        DirectoryEntry de;
        fk::memory::copy_n(de.name, entry.name.c_str(), sizeof(de.name) - 1);
        de.name[sizeof(de.name) - 1] = '\0';
        
        if (entry.node->is_directory()) {
            de.type = 1; // DT_DIR
        } else if (entry.node->is_symlink()) {
            de.type = 2; // DT_LNK
        } else {
            de.type = 0; // DT_REG (or block/char, but we use 0 for now)
        }
        
        entries.push_back(de);
    }
    return {};
}

} // namespace fkernel
