#pragma once

#include <Kernel/Driver/Device/device_id.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Container/hash_map.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Memory/own_ptr.h>

namespace fkernel {

class Driver {
public:
    virtual ~Driver() = default;
    virtual const char* name() const = 0;
    virtual void probe() = 0;
};

class DriverManager {
public:
    static DriverManager& the();

    fk::core::Result<void, fk::core::Error> register_driver(fk::memory::OwnPtr<Driver> driver);
    fk::core::Result<void, fk::core::Error> register_device(fk::RefPtr<Node> device);
    void unregister_device(fk::RefPtr<Node> device);

    void probe_all();

private:
    DriverManager() = default;

    fk::containers::Vector<fk::memory::OwnPtr<Driver>> m_drivers;
    fk::containers::Vector<fk::RefPtr<Node>> m_devices;
};

} // namespace fkernel
