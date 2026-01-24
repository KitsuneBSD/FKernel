#include <Kernel/Fs/DevFs/dev_fs.h>
#include <LibFK/Algorithms/log.h>

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

    // Verifica se já existe
    for (auto& entry : m_devices) {
        if (entry.name == name) return fk::core::Error::PermissionDenied;
    }

    m_devices.push_back({name, node});
    fk::algorithms::klog("DEVFS", "Registered device: /dev/%s", name);
    
    // TODO: Aqui poderíamos disparar um evento estilo udev para o userspace
    return {};
}

fk::core::Result<void, fk::core::Error> DevFs::unregister_device(const char* name) {
    for (size_t i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].name == name) {
            // Em um sistema real, removeríamos da lista. 
            // Nossa Vector precisa de um método remove_at ou similar.
            // Por enquanto, apenas invalidamos.
            m_devices[i].node = nullptr;
            return {};
        }
    }
    return fk::core::Error::NotFound;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> DevFs::lookup(const char* name) {
    for (auto& entry : m_devices) {
        if (entry.name == name && entry.node) {
            return entry.node;
        }
    }
    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error> DevFs::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    for (auto& entry : m_devices) {
        if (!entry.node) continue;
        
        DirectoryEntry de;
        strncpy(de.name, entry.name.c_str(), sizeof(de.name) - 1);
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
