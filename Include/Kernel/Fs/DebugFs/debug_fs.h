#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class DebugLogNode final : public Node {
public:
    DebugLogNode();
    virtual ~DebugLogNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t* buffer) override;
    virtual size_t size() const override;

    void append(const char* str, size_t len);

    static fk::RefPtr<DebugLogNode> the();

private:
    fk::containers::Vector<uint8_t> m_buffer;
    static constexpr size_t MAX_LOG_SIZE = 64 * 1024; // 64KB log buffer
};

class SyscallLogNode final : public Node {
public:
    SyscallLogNode();
    virtual ~SyscallLogNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t* buffer) override;
    virtual size_t size() const override;

    void append(const char* str, size_t len);

    static fk::RefPtr<SyscallLogNode> the();

private:
    fk::containers::Vector<uint8_t> m_buffer;
    static constexpr size_t MAX_LOG_SIZE = 128 * 1024; // 128KB log buffer
};

class DebugFsNode final : public Node {
public:
    DebugFsNode();
    virtual ~DebugFsNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::IsDirectory; }
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
    virtual size_t size() const override { return 0; }

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    virtual bool is_directory() const override { return true; }
};

} // namespace fkernel
