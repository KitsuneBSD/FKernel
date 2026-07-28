#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Vfs/dentry_node_stack.h>
#include <Kernel/Fs/Vfs/mount_namespace.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Text/string.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

class Dentry : public fk::memory::RefCounted<Dentry> {
public:
    static fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error> create(fk::text::String name, fk::RefPtr<Dentry> parent = nullptr);

    const fk::text::String& name() const { return m_name; }
    fk::RefPtr<Dentry> parent() const { return m_parent; }

    // Node Stack (Union Support)
    void push_node(fk::RefPtr<Node> node);
    void pop_node();
    fk::RefPtr<Node> top_node() const;
    const fk::containers::Vector<fk::RefPtr<Node>>& nodes() const;
    DentryNodeStack& default_stack() { return m_node_stack; }

    // Child Management
    fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error> lookup(const char* name);
    void add_child(fk::RefPtr<Dentry> child);
    fk::containers::Vector<fk::RefPtr<Dentry>>& children() { return m_children; }

    template <typename Fn>
    void for_each_child(Fn&& fn) const {
        fk::synchronization::ScopedLock lock(m_lock);
        for (auto& child : m_children)
            fn(child);
    }

    fk::text::String get_path() const;

    Dentry(fk::text::String name, fk::RefPtr<Dentry> parent);

private:
    mutable fk::synchronization::Spinlock m_lock;
    fk::text::String m_name;
    fk::RefPtr<Dentry> m_parent;
    DentryNodeStack m_node_stack;
    fk::containers::Vector<fk::RefPtr<Dentry>> m_children;
};

} // namespace fkernel
