#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Mount/mount_namespace.h>
#include <LibFK/Memory/Allocators/new.h>
#include <LibFK/Utilities/memory.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

static DentryNodeStack& dentry_stack(Dentry* d) {
  auto* ns = current_mount_namespace();
  if (ns) {
    auto* ns_stack = ns->get_stack(d);
    if (ns_stack) return *ns_stack;
  }
  return d->default_stack();
}

Dentry::Dentry(fk::text::String name, fk::RefPtr<Dentry> parent)
    : m_name(fk::types::move(name)), m_parent(fk::types::move(parent)) {}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error> Dentry::create(fk::text::String name, fk::RefPtr<Dentry> parent) {
    return fk::make_ref<Dentry>(fk::types::move(name), fk::types::move(parent)).value();
}

void Dentry::push_node(fk::RefPtr<Node> node) {
    if (!node) return;
    auto* ns = current_mount_namespace();
    if (ns) {
      ns->ensure_stack(this).push(node);
      return;
    }
    fk::synchronization::ScopedLock lock(m_lock);
    m_node_stack.push(node);
}

void Dentry::pop_node() {
    auto* ns = current_mount_namespace();
    if (ns) {
      auto* ns_stack = ns->get_stack(this);
      if (ns_stack) ns_stack->pop();
      return;
    }
    fk::synchronization::ScopedLock lock(m_lock);
    m_node_stack.pop();
}

fk::RefPtr<Node> Dentry::top_node() const {
    auto* ns = current_mount_namespace();
    if (ns) {
      auto* ns_stack = ns->get_stack(const_cast<Dentry*>(this));
      if (ns_stack) return ns_stack->top();
      return nullptr;
    }
    fk::synchronization::ScopedLock lock(m_lock);
    return m_node_stack.top();
}

const fk::containers::Vector<fk::RefPtr<Node>>& Dentry::nodes() const {
    auto* ns = current_mount_namespace();
    if (ns) {
      auto* ns_stack = ns->get_stack(const_cast<Dentry*>(this));
      if (ns_stack) return ns_stack->all();
    }
    return m_node_stack.all();
}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error> Dentry::lookup(const char* name) {
    if (fk::memory::compare(name, ".") == 0) return fk::RefPtr<Dentry>(this);
    if (fk::memory::compare(name, "..") == 0) return m_parent ? m_parent : fk::RefPtr<Dentry>(this);

    {
        fk::synchronization::ScopedLock lock(m_lock);
        auto cached = m_child_map.get(fk::text::String(name));
        if (cached.has_value() && cached.value())
            return fk::RefPtr<Dentry>(cached.value());
    }

    const auto& all_nodes = dentry_stack(this).all();
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
        // Re-check under lock (another thread may have raced us)
        auto cached = m_child_map.get(fk::text::String(name));
        if (cached.has_value() && cached.value())
            return fk::RefPtr<Dentry>(cached.value());
        m_child_map.insert(new_dentry->name(), new_dentry.get());
        m_children.push_back(new_dentry);
        return new_dentry;
    }

    return fk::core::Error::NotFound;
}

void Dentry::add_child(fk::RefPtr<Dentry> child) {
    fk::synchronization::ScopedLock lock(m_lock);
    m_child_map.insert(child->name(), child.get());
    m_children.push_back(child);
}

fk::text::String Dentry::get_path() const {
    if (!m_parent) return m_name.is_empty() ? "/" : m_name;
    fk::text::String parent_path = m_parent->get_path();
    if (parent_path == "/") return "/" + m_name;
    return parent_path + "/" + m_name;
}

} // namespace fkernel
