#pragma once
#include <Kernel/Fs/Virtual/TmpFs/tmp_fs_child.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Container/Sequence/vector.h>

class ChildList {
  fk::containers::Vector<Child> m_entries;

public:
  fk::RefPtr<Node> find_by_name(const char* name);
  void add(Child child) { m_entries.push_back(child); }
  const fk::containers::Vector<Child>& entries() const { return m_entries; }
  size_t size() const { return m_entries.size(); }
  bool remove_by_name(const char* name) {
    for (size_t i = 0; i < m_entries.size(); i++) {
      if (m_entries[i].has_name(name)) {
        if (i < m_entries.size() - 1) {
          m_entries[i] = fk::types::move(m_entries[m_entries.size() - 1]);
        }
        m_entries.pop_back();
        return true;
      }
    }
    return false;
  }
};
