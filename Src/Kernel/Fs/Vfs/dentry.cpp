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
    if (!node) return;
    fk::synchronization::ScopedLock lock(m_lock);
    m_node_stack.push(node);
}

void Dentry::pop_node() {
    fk::synchronization::ScopedLock lock(m_lock);
    m_node_stack.pop();
}

fk::RefPtr<Node> Dentry::top_node() const {
    fk::synchronization::ScopedLock lock(m_lock);
    return m_node_stack.top();
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
    const auto& all_nodes = m_node_stack.all();
    for (int i = static_cast<int>(all_nodes.size()) - 1; i >= 0; --i) {
        auto res = all_nodes[i]->lookup(name);
        if (!res.is_ok())
            continue;

        auto new_dentry = TRY(Dentry::create(name, this));
        new_dentry->push_node(res.value());

        for (int j = i - 1; j >= 0; --j) {
            auto sub_res = all_nodes[j]->lookup(name);
            if (sub_res.is_ok())
                new_dentry->push_node(sub_res.value());
        }

        fk::synchronization::ScopedLock lock(m_lock);
        for (auto& child : m_children) {
            if (child->name() == name) return child;
        }
        m_children.push_back(new_dentry);
        return new_dentry;
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
