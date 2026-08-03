#pragma once
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Text/string.h>

class Child {
  fk::text::String m_name;
  fk::RefPtr<Node> m_node;

public:
  Child(fk::text::String name, fk::RefPtr<Node> node) : m_name(name), m_node(node) {}

  bool has_name(const char* name) const { return m_name == name; }
  const fk::text::String& name() const { return m_name; }
  fk::RefPtr<Node> node() const { return m_node; }
};
