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
class CORTEX_API SuffixTree {
 public:
  typedef K Key;
  typedef typename Key::value_type Elem;
  typedef V Value;

  SuffixTree();
  ~SuffixTree();

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
SuffixTree<K, V>::SuffixTree()
    : root_() {}

template <typename K, typename V>
SuffixTree<K, V>::~SuffixTree() {}

template <typename K, typename V>
void SuffixTree<K, V>::insert(const Key& key, const Value& value) {
  Node* level = &root_;

  // insert reverse
  for (auto i = key.rbegin(), e = key.rend(); i != e; ++i)
    level = acquire(*i, level);

  level->value = value;
}

template <typename K, typename V>
typename SuffixTree<K, V>::Node* SuffixTree<K, V>::acquire(Elem elem, Node* n) {
  auto i = n->children.find(elem);
  if (i != n->children.end()) return i->second;

  Node* c = new Node(n, elem);
  n->children[elem] = c;
  return c;
}

template <typename K, typename V>
bool SuffixTree<K, V>::lookup(const Key& key, Value* value) const {
  const Node* level = &root_;

  for (auto i = key.rbegin(), e = key.rend(); i != e; ++i) {
    auto k = level->children.find(*i);
    if (k == level->children.end()) break;

    level = k->second;
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
