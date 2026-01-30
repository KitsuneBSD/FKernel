#include <Kernel/Fs/Vfs/dentry.h>
#include <LibFK/Memory/new.h>
#include <LibC/string.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

Dentry::Dentry(fk::text::String name, fk::RefPtr<Dentry> parent)
    : m_name(fk::types::move(name)), m_parent(fk::types::move(parent)) {}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error> Dentry::create(fk::text::String name, fk::RefPtr<Dentry> parent) {
    return fk::make_ref<Dentry>(fk::types::move(name), fk::types::move(parent)).value();
}

void Dentry::push_node(fk::RefPtr<Node> node) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (node) {
        m_nodes.push_back(node);
    }
}

void Dentry::pop_node() {
    fk::synchronization::ScopedLock lock(m_lock);
    if (!m_nodes.is_empty()) {
        m_nodes.pop_back();
    }
}

fk::RefPtr<Node> Dentry::top_node() const {
    // We need to lock even for read because nodes can be pushed/popped
    // But since it's a const method, we need a mutable lock or cast.
    // I'll make the lock mutable in the header.
    auto& mutable_this = const_cast<Dentry&>(*this);
    fk::synchronization::ScopedLock lock(mutable_this.m_lock);
    if (m_nodes.is_empty()) return nullptr;
    return m_nodes[m_nodes.size() - 1];
}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error> Dentry::lookup(const char* name) {
    if (strcmp(name, ".") == 0) return fk::RefPtr<Dentry>(this);
    if (strcmp(name, "..") == 0) return m_parent ? m_parent : fk::RefPtr<Dentry>(this);

    {
        fk::synchronization::ScopedLock lock(m_lock);
        for (auto& child : m_children) {
            if (child->name() == name) return child;
        }
    }

    // Not in cache, try lookups in the node stack
    // We keep m_lock unlocked during I/O lookup to allow parallelism
    // and avoid deadlocks if lookup() calls VFS.
    for (int i = static_cast<int>(m_nodes.size()) - 1; i >= 0; --i) {
        auto res = m_nodes[i]->lookup(name);
        if (res.is_ok()) {
            auto new_dentry = TRY(Dentry::create(name, this));
            new_dentry->push_node(res.value());
            
            // Check if this new dentry itself shadows other nodes in the underlying stack
            for (int j = i - 1; j >= 0; --j) {
                auto sub_res = m_nodes[j]->lookup(name);
                if (sub_res.is_ok()) {
                    new_dentry->push_node(sub_res.value());
                }
            }
            
            fk::synchronization::ScopedLock lock(m_lock);
            m_children.push_back(new_dentry);
            return new_dentry;
        }
    }

    return fk::core::Error::NotFound;
}

void Dentry::add_child(fk::RefPtr<Dentry> child) {
    fk::synchronization::ScopedLock lock(m_lock);
    m_children.push_back(child);
}

fk::text::String Dentry::get_path() const {
    if (!m_parent) return m_name.is_empty() ? "/" : m_name;
    fk::text::String parent_path = m_parent->get_path();
    if (parent_path == "/") return "/" + m_name;
    return parent_path + "/" + m_name;
}

} // namespace fkernel
