// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <unordered_map>

namespace cortex {

template <typename K, typename V>
class CORTEX_API PrefixTree {
 public:
  typedef K Key;
  typedef typename Key::value_type Elem;
  typedef V Value;

  PrefixTree();
  ~PrefixTree();

  void insert(const Key& key, const Value& value);
  bool lookup(const Key& key, Value* value) const;

 private:
  struct Node {  // {{{
    Node* parent;
    Elem element;
    std::unordered_map<Elem, Node*> children;
    Value value;

    Node() : parent(nullptr), element(), children(), value() {}
    Node(Node* p, Elem e) : parent(p), element(e), children(), value() {}

    ~Node() {
      for (auto& n : children) {
        delete n.second;
      }
    }

    Node** get(Elem e) {
      auto i = children.find(e);
      if (i != children.end()) return &i->second;
      return &children[e];
    }
  };  // }}}

  Node root_;

  Node* acquire(Elem el, Node* n);
};

// {{{
template <typename K, typename V>
PrefixTree<K, V>::PrefixTree()
    : root_() {}

template <typename K, typename V>
PrefixTree<K, V>::~PrefixTree() {}

template <typename K, typename V>
void PrefixTree<K, V>::insert(const Key& key, const Value& value) {
  Node* level = &root_;

  for (const auto& ke : key) level = acquire(ke, level);

  level->value = value;
}

template <typename K, typename V>
typename PrefixTree<K, V>::Node* PrefixTree<K, V>::acquire(Elem elem, Node* n) {
  auto i = n->children.find(elem);
  if (i != n->children.end()) return i->second;

  Node* c = new Node(n, elem);
  n->children[elem] = c;
  return c;
}

template <typename K, typename V>
bool PrefixTree<K, V>::lookup(const Key& key, Value* value) const {
  const Node* level = &root_;

  for (const auto& ke : key) {
    auto i = level->children.find(ke);
    if (i == level->children.end()) break;

    level = i->second;
  }

  while (level && level->parent) {
    if (level->value) {
      *value = level->value;
      return true;
    }
    level = level->parent;
  }

  return false;
}
// }}}

}  // namespace cortex
