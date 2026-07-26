#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/Archive/tar.h>

namespace fkernel {

class RamFileNode final : public Node {
public:
    RamFileNode(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::PermissionDenied; // Read-only
    }
    virtual size_t size() const override { return m_size; }

private:
    const uint8_t* m_data;
    size_t m_size;
};

class RamSymlinkNode final : public Node {
public:
    RamSymlinkNode(const fk::text::String& target) : m_target(target) {}

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        return fk::core::Error::IsASymlink;
    }
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::PermissionDenied;
    }
    virtual size_t size() const override { return m_target.length(); }

    virtual bool is_symlink() const override { return true; }
    virtual fk::core::Result<fk::text::String, fk::core::Error> read_link() override {
        return m_target;
    }

private:
    fk::text::String m_target;
};

class RamDiskNode final : public Node {
public:
    static fk::core::Result<fk::RefPtr<RamDiskNode>, fk::core::Error> create(uintptr_t start, uintptr_t end);

    RamDiskNode(uintptr_t start, uintptr_t end) : m_start(start), m_end(end) {}

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        return fk::core::Error::IsDirectory;
    }
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::PermissionDenied; // Read-only
    }
    virtual size_t size() const override { return 0; }

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    virtual bool is_directory() const override { return true; }
    virtual fk::text::String get_path() const override { 
        if (m_prefix.is_empty()) return "/";
        fk::text::String p = "/";
        const char* pref = m_prefix.c_str();
        size_t len = strlen(pref);
        if (len > 0 && pref[len-1] == '/') len--;
        for(size_t i=0; i<len; ++i) p.push_back(pref[i]);
        return p;
    }

private:
    void parse_tar();

    struct Entry {
        fk::text::String name;
        fk::RefPtr<Node> node;
    };

    uintptr_t m_start;
    uintptr_t m_end;
    fk::text::String m_prefix{""};
    fk::containers::Vector<Entry> m_files;
    fk::containers::Vector<fk::text::String> m_directories;
};

} // namespace fkernel