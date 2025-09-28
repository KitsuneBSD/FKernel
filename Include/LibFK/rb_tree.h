#pragma once

#include <LibFK/log.h>
#include <LibFK/new.h>

template <typename T> class rb_node {
private:
  bool is_red = true;
  T value;

  rb_node<T> *parent;
  rb_node<T> *left;
  rb_node<T> *right;

public:
  explicit rb_node(T new_value)
      : is_red(true), value(new_value), parent(nullptr), left(nullptr),
        right(nullptr) {}

  rb_node<T>() {
    is_red = true;

    parent = nullptr;
    left = nullptr;
    right = nullptr;
  }

  T get_value() { return value; }
  bool get_color() { return is_red; }

  void change_color() { is_red = !is_red; }

  rb_node<T> *get_parent() { return this->parent; }
  rb_node<T> *get_right() { return this->right; }
  rb_node<T> *get_left() { return this->left; }

  void set_color(bool color) { is_red = color; }
  void set_parent(rb_node<T> *node) { parent = node; }
  void set_right(rb_node<T> *node) {
    right = node;
    if (node)
      node->parent = this;
  }
  void set_left(rb_node<T> *node) {
    left = node;

    if (node)
      node->parent = this;
  }
};

template <typename T, typename Compare = bool (*)(const T &, const T &)>
class rb_tree {
private:
  rb_node<T> *m_root{nullptr};
  Compare cmp;

  void rotate_left(rb_node<T> *x) {
    rb_node<T> *y = x->get_right();
    x->set_right(y->get_left());

    if (x->get_parent() == nullptr)
      m_root = y;
    else if (x == x->get_parent()->get_left())
      x->get_parent()->set_left(y);
    else
      x->get_parent()->set_right(y);

    y->set_left(x);
  }

  void rotate_right(rb_node<T> *x) {
    rb_node<T> *y = x->get_left();
    x->set_left(y->get_right());
    if (x->get_parent() == nullptr)
      m_root = y;
    else if (x == x->get_parent()->get_right())
      x->get_parent()->set_right(y);
    else
      x->get_parent()->set_left(y);

    y->set_right(x);
  }

  void rebalance(rb_node<T> *node) {
    while (node->get_parent() && node->get_parent()->get_color()) {
      rb_node<T> *parent = node->get_parent();
      rb_node<T> *grandparent = parent->get_parent();

      if (parent == grandparent->get_left()) {
        rb_node<T> *uncle = grandparent->get_right();
        if (uncle && uncle->get_color()) {
          parent->change_color();
          uncle->change_color();
          grandparent->change_color();
          node = grandparent;
        } else {
          if (node == parent->get_right()) {
            node = parent;
            rotate_left(node);
          }
          parent->change_color();
          grandparent->change_color();
          rotate_right(grandparent);
        }
      } else {
        rb_node<T> *uncle = grandparent->get_left();
        if (uncle && uncle->get_color()) {
          parent->change_color();
          uncle->change_color();
          grandparent->change_color();
          node = grandparent;
        } else {
          if (node == parent->get_left()) {
            node = parent;
            rotate_right(node);
          }
          parent->change_color();
          grandparent->change_color();
          rotate_left(grandparent);
        }
      }
    }
    m_root->set_color(false);
  }

public:
  explicit rb_tree(Compare cmp_func = [](const T &a,
                                         const T &b) { return a < b; })
      : cmp(cmp_func) {}

  void insert(T node) { insert_on_tree(new rb_node<T>(node)); }

  void insert_on_tree(rb_node<T> *node) {
    rb_node<T> *y = nullptr;
    rb_node<T> *x = m_root;

    while (x) {
      y = x;
      if (cmp(node->get_value(), x->get_value()))
        x = x->get_left();
      else
        x = x->get_right();
    }

    node->set_parent(y);
    if (!y)
      m_root = node;
    else if (cmp(node->get_value(), y->get_value()))
      y->set_left(node);
    else
      y->set_right(node);

    node->set_left(nullptr);
    node->set_right(nullptr);
    node->set_color(true);

    rebalance(node);
  }

  rb_node<T> *find(const T &value) const {
    rb_node<T> *current = m_root;
    while (current) {
      if (cmp(value, current->get_value()))
        current = current->get_left();
      else if (cmp(current->get_value(), value))
        current = current->get_right();
      else
        return current;
    }
    return nullptr;
  }

  rb_node<T> *get_root() const { return m_root; }
};
