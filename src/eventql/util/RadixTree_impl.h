/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
namespace util {

template <typename T>
RadixTree<T>::RadixTree() {
  if (art_tree_init(&tree_) != 0) {
    RAISE(kRuntimeError, "art_tree_init() failed");
  }
}

template <typename T>
RadixTree<T>::~RadixTree() {
  art_tree_destroy(&tree_);
}

template <typename T>
void RadixTree<T>::insert(const String& key, T value) {
  art_insert(
      &tree_,
      (unsigned char *) key.c_str(),
      key.length(),
      (void*) value);
}

template <typename T>
Option<T> RadixTree<T>::get(const String& key) const {
  auto res = art_search(
      &tree_,
      (unsigned char *) key.c_str(),
      key.length());

  if (res == nullptr) {
    return None<T>();
  } else {
    return Some((T) res);
  }
}

template <typename T>
uint64_t RadixTree<T>::size() const {
  return art_size(&tree_);
}

}
