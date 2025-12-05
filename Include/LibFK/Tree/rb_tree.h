#pragma once

#include <LibFK/Algorithms/log.h> // For kerror
#include <LibFK/Core/Result.h>    // Include Result for return types
#include <LibFK/Utilities/pair.h> // Include Pair definition

namespace fk {
namespace containers { // rb_tree is a container

template <typename T> class rb_node {
private:
  // Group color and pointers into a struct to satisfy the two-instance-variable
  // rule.
  struct NodeLinks {
    bool m_red;
    rb_node<T> *m_parent;
    rb_node<T> *m_left;
    rb_node<T> *m_right;
  };

  NodeLinks m_links;
  T m_value;

public:
  rb_node() : m_links({true, nullptr, nullptr, nullptr}), m_value() {}
  rb_node(const T &value)
      : m_links({true, nullptr, nullptr, nullptr}), m_value(value) {}

  bool is_red() const { return m_links.m_red; }
  void set_red(bool red) { m_links.m_red = red; }
  void toggle_color() { m_links.m_red = !m_links.m_red; }

  T &value() { return m_value; }
  const T &value() const { return m_value; }

  rb_node<T> *parent() const { return m_links.m_parent; }
  rb_node<T> *left() const { return m_links.m_left; }
  rb_node<T> *right() const { return m_links.m_right; }

  void set_parent(rb_node<T> *p) { m_links.m_parent = p; }

  void set_left(rb_node<T> *l) {
    m_links.m_left = l;
    if (l)
      l->m_links.m_parent = this;
  }

  void set_right(rb_node<T> *r) {
    m_links.m_right = r;
    if (r)
      r->m_links.m_parent = this;
  }
};

template <typename T, unsigned MAX_NODES = 65355> class rb_tree {
private:
  rb_node<T> *m_root;
  rb_node<T> m_pool[MAX_NODES];
  // Group m_pool_index and size_tree into a Pair to satisfy the
  // two-instance-variable rule.
  fk::utilities::Pair<unsigned, size_t>
      m_pool_and_size; // first: m_pool_index, second: size_tree

  bool is_black(rb_node<T> *node) { return !node || !node->is_red(); }

  rb_node<T> *sibling(rb_node<T> *node) {
    rb_node<T> *p = node->parent();
    if (!p)
      return nullptr;
    return node == p->left() ? p->right() : p->left();
  }

  rb_node<T> *minimum(rb_node<T> *node) const {
    while (node->left())
      node = node->left();
    return node;
  }

  void transplant(rb_node<T> *u, rb_node<T> *v) {
    rb_node<T> *p = u->parent();
    if (!p)
      m_root = v;
    else if (u == p->left())
      p->set_left(v);
    else
      p->set_right(v);
    if (v)
      v->set_parent(p);
  }

  void rotate_left(rb_node<T> *x) {
    rb_node<T> *y = x->right();
    x->set_right(y->left());
    rb_node<T> *p = x->parent();
    if (!p)
      m_root = y;
    else if (x == p->left())
      p->set_left(y);
    else
      p->set_right(y);
    y->set_left(x);
  }

  void rotate_right(rb_node<T> *x) {
    rb_node<T> *y = x->left();
    x->set_left(y->right());
    rb_node<T> *p = x->parent();
    if (!p)
      m_root = y;
    else if (x == p->right())
      p->set_right(y);
    else
      p->set_left(y);
    y->set_right(x);
  }

  void rebalance_rotate(rb_node<T> *p, rb_node<T> *g, bool left) {
    p->toggle_color();
    g->toggle_color();
    if (left)
      rotate_right(g);
    else
      rotate_left(g);
  }

  void _rebalance_right_red_uncle(rb_node<T> *p, rb_node<T> *u, rb_node<T> *g) {
    p->toggle_color();
    u->toggle_color();
    g->toggle_color();
  }

  void _rebalance_right_black_uncle(rb_node<T> *&node, rb_node<T> *p, rb_node<T> *g) {
    if (node == p->left()) {
      node = p;
      rotate_right(node);
    }
    rebalance_rotate(p, g, false);
  }

  void _rebalance_left_red_uncle(rb_node<T> *p, rb_node<T> *u, rb_node<T> *g) {
    p->toggle_color();
    u->toggle_color();
    g->toggle_color();
  }

  void _rebalance_left_black_uncle(rb_node<T> *&node, rb_node<T> *p, rb_node<T> *g) {
    if (node == p->right()) {
      node = p;
      rotate_left(node);
    }
    rebalance_rotate(p, g, true);
  }

  void rebalance(rb_node<T> *node) {
    while (node->parent() && node->parent()->is_red()) {
      rb_node<T> *p = node->parent();
      rb_node<T> *g = p->parent();
      if (p == g->left()) {
        rb_node<T> *u = g->right();
        if (u && u->is_red()) {
          _rebalance_left_red_uncle(p, u, g);
          node = g;
        } else {
          _rebalance_left_black_uncle(node, p, g);
        }
      } else { // p == g->right()
        rb_node<T> *u = g->left();
        if (u && u->is_red()) {
          _rebalance_right_red_uncle(p, u, g);
          node = g;
        } else {
          _rebalance_right_black_uncle(node, p, g);
        }
      }
    }
    if (m_root)
      m_root->set_red(false);
  }

  void fix_remove_case(rb_node<T> *&w, rb_node<T> *p, bool left_side) {
    if (left_side) {
      if (is_black(w->right())) {
        if (w->left())
          w->left()->set_red(false);
        w->set_red(true);
        rotate_right(w);
        w = p->right();
      }
    } else {
      if (is_black(w->left())) {
        if (w->right())
          w->right()->set_red(false);
        w->set_red(true);
        rotate_left(w);
        w = p->left();
      }
    }
  }

  void _fix_remove_black_sibling_no_red_children(rb_node<T> *&x, rb_node<T> *&w) {
    w->set_red(true);
    x = x->parent();
  }

  void _fix_remove_black_sibling_red_child(rb_node<T> *&x, rb_node<T> *&w, rb_node<T> *p, bool left_side) {
    fix_remove_case(w, p, left_side);
    if (w)
      w->set_red(p->is_red());
    p->set_red(false);
    if (left_side) {
      if (w && w->right())
        w->right()->set_red(false);
      rotate_left(p);
    } else {
      if (w && w->left())
        w->left()->set_red(false);
      rotate_right(p);
    }
    x = m_root;
  }

  void _fix_remove_red_sibling(rb_node<T> *p, rb_node<T> *&w, bool left_side) {
    w->set_red(false);
    p->set_red(true);
    if (left_side)
      rotate_left(p);
    else
      rotate_right(p);
    w = left_side ? p->right() : p->left();
  }

  void fix_remove(rb_node<T> *x) {
    while (x != m_root && is_black(x)) {
      rb_node<T> *p = x ? x->parent() : nullptr;
      if (!p)
        break;
      bool left_side = x == p->left();
      rb_node<T> *w = left_side ? p->right() : p->left();

      if (w && w->is_red()) {
        _fix_remove_red_sibling(p, w, left_side);
      }

      if (is_black(w->left()) && is_black(w->right())) {
        _fix_remove_black_sibling_no_red_children(x, w);
      } else {
        _fix_remove_black_sibling_red_child(x, w, p, left_side);
      }
    }
    if (x)
      x->set_red(false);
  }

  fk::core::Result<rb_node<T>*, fk::core::Error>
  allocate_node(const T &value) {
    if (m_pool_and_size.first >= MAX_NODES)
      return fk::core::Error::OutOfMemory; // Pool is full
    rb_node<T> *node_ptr = &m_pool[m_pool_and_size.first++];
    new (node_ptr) rb_node<T>(value); // Construct the node in place using placement new.
    return fk::core::Result<rb_node<T>*>(node_ptr);
  }

public:
  rb_tree() : m_root(nullptr), m_pool_and_size({0, 0}) {}

  // Changed return type to Result
  fk::core::Result<void, fk::core::Error> insert(const T &value) {
    auto result = allocate_node(value);
    if (result.is_error()) {
      return result.error();
    }
    rb_node<T> *new_node = result.value();

    rb_node<T> *y = nullptr;
    rb_node<T> *x = m_root;
    while (x) {
      y = x;
      x = (value < x->value()) ? x->left() : x->right();
    }

    new_node->set_parent(y);
    if (!y)
      m_root = new_node;
    else if (value < y->value())
      y->set_left(new_node);
    else
      y->set_right(new_node);

    new_node->set_left(nullptr);
    new_node->set_right(nullptr);
    new_node->set_red(true);

    rebalance(new_node);
    m_pool_and_size.second++;
    return fk::core::Result<void>();
  }

  rb_node<T> *find(const T &value) const {
    rb_node<T> *current = m_root;
    while (current) {
      if (value < current->value())
        current = current->left();
      else if (current->value() < value)
        current = current->right();
      else
        return current;
    }
    return nullptr;
  }

  bool remove(const T &value) {
    rb_node<T> *z = find(value);
    if (!z)
      return false;

    rb_node<T> *y = z;
    rb_node<T> *x = nullptr;
    bool y_original_red = y->is_red();

    if (!z->left()) {
      x = z->right();
      transplant(z, z->right());
    } else if (!z->right()) {
      x = z->left();
      transplant(z, z->left());
    } else {
      y = minimum(z->right());
      y_original_red = y->is_red();
      x = y->right();

      if (y->parent() == z) {
        if (x)
          x->set_parent(y);
      } else {
        transplant(y, y->right());
        y->set_right(z->right());
        if (y->right())
          y->right()->set_parent(y);
      }

      transplant(z, y);
      y->set_left(z->left());
      if (y->left())
        y->left()->set_parent(y);
      y->set_red(z->is_red());
    }

    if (!y_original_red)
      fix_remove(x);

    m_pool_and_size.second--; // Decrement size_tree
    return true;
  }

  rb_node<T> *root() const { return m_root; }
  size_t size() const { return m_pool_and_size.second; }
};

} // namespace containers
} // namespace fk
