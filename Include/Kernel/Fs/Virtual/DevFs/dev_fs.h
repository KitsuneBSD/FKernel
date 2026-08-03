#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/pair.h>

namespace fkernel {

class DevFs final : public Node {
private:
    struct DeviceEntry {
        fk::text::String name;
        fk::RefPtr<Node> node;
    };

    fk::containers::Vector<DeviceEntry> m_devices;
    mutable fk::synchronization::Spinlock m_lock;

public:
    DevFs();
    static DevFs& the();

    // API para Drivers (Inspirado em make_dev do BSD)
    fk::core::Result<void, fk::core::Error> register_device(fk::RefPtr<Node> node, const char* name);
    fk::core::Result<void, fk::core::Error> unregister_device(const char* name);

    // Overrides de Node para comportamento de diretório
    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        return fk::core::Error::NotADirectory;
    }
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::NotADirectory;
    }
    virtual size_t size() const override { return m_devices.size(); }

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    virtual bool is_directory() const override { return true; }

    // TODO: Implementar readdir para permitir 'ls /dev'
};

} // namespace fkernel
